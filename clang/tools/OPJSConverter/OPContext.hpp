//
//  OPContext.hpp
//  OPJSConverter
//
//  Created by Zhiyang Fu on 2021/3/16.
//

#ifndef OPContext_hpp
#define OPContext_hpp

#include <iostream>
#include <vector>
#include "llvm/Support/Casting.h"

using namespace std;

class OPParam
{
public:
    char *type;
    void *value;
};

class OPContext {
public:
    /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
    enum ContextKind {
        ContextKind_MethodDecl,
        ContextKind_ImplementationDecl,
        ContextKind_VarDecl,
        ContextKind_MessageExpr,
        ContextKind_IfStmt,
        ContextKind_StringLiteral,
        ContextKind_IntegerLiteral,
        ContextKind_FloatingLiteral,
        ContextKind_CallExpr,
        ContextKind_CompoundStmt,
    };
private:
      const ContextKind Kind;
public:
    ContextKind getKind() const { return Kind; }
    OPContext(ContextKind K): Kind(K) {}
    
public:
    OPContext *m_next = NULL;
    OPContext *m_pre = NULL;
    OPContext *m_parent = NULL;
    
    virtual ~OPContext(){}
    
    void setNext(OPContext *ctx) {
        if (m_next) {
            ctx->m_pre = this->m_next;
            m_next->m_next = ctx;
        } else {
            ctx->m_pre = this;
            m_next = ctx;
        }
    }
    
    virtual string parse() {
        return "";
    }
};

// - (void)xxx{}
// + (void)xxx{}
class OPMethodDeclContext: public OPContext {
public:
    OPMethodDeclContext() : OPContext(ContextKind_MethodDecl) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_MethodDecl;
    }
    
public:
    string m_className;
    string m_methodName;
    bool m_isClassMethod;
    vector<string> m_params;
    
    string parse() {
        string script = "";
        if (m_isClassMethod) {
            script += "static ";
        }
        
        script += m_methodName + " (";
        if (m_params.size() > 0) {
            for (vector<string>::iterator itr = m_params.begin(); itr != m_params.end(); itr++) {
                script += *itr;
                if (itr != m_params.end() - 1) {
                    script += ", ";
                }
            }
        }
        script += ") { \n";
        
        OPContext *nextCtx = m_next;
        while (nextCtx) {
            script += nextCtx->parse();
            nextCtx = nextCtx->m_next;
        }
        
        script += "\n} \n";
        
        return script;
    }
};

// @implementation xx
class OPImplementationDeclContext: public OPContext {
public:
    OPImplementationDeclContext() : OPContext(ContextKind_ImplementationDecl) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_ImplementationDecl;
    }
    
public:
    string m_className;
    vector<OPMethodDeclContext *> m_methodContexts;
    
    string parse() {
        string script = "class ";
        
        script = script + m_className + " { \n";
        
        for (vector<OPMethodDeclContext *>::iterator i = m_methodContexts.begin(), e = m_methodContexts.end(); i != e; i++) {
            OPMethodDeclContext *p = *i;
            
            script += p->parse();
            script += "\n";
        }
        
        script += "\n} \n";
        
        return script;
    }
};

class OPVarDeclContext: public OPContext {
public:
    OPVarDeclContext() : OPContext(ContextKind_VarDecl) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_VarDecl;
    }
    
public:
    string m_varName;
    OPContext *m_initContext;
    
    string parse() {
        string script = "var " + m_varName + " = ";
        if (m_initContext) {
            script += m_initContext->parse();
        }
        script += ';';
        
        return script;
    }
};

// [self a]
class OPMessageExprContext: public OPContext {
public:
    OPMessageExprContext() : OPContext(ContextKind_MessageExpr) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_MessageExpr;
    }
    
public:
    bool m_isClassMethod = false;
    string m_className;
    string m_selName;
    vector<OPParam> m_params;
    string m_receiverName;
    OPContext *m_receiverContext = NULL;
    
    string parse() {
        string script = "";
        
        if (m_receiverContext) {
            script += m_receiverContext->parse();
            script += ".";
        }
        
        if (m_isClassMethod) {
            script += "require('";
            script += m_className;
            script += "').";
        } else {
            if (m_receiverName.length() != 0) {
                script += m_receiverName;
                script += ".";
            }
        }
        
        script += m_selName;
        script += "(";
        
        if (m_params.size() > 0) {
            for (vector<OPParam>::iterator itr = m_params.begin(); itr != m_params.end(); itr++) {
                script += ",";
                OPParam param = *itr;
//                script += param.value;
            }
        }
        
        script += ")";
        
        return script;
    }
};

class OPCompoundStmtContext: public OPContext {
public:
    OPCompoundStmtContext() : OPContext(ContextKind_CompoundStmt) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_CompoundStmt;
    }
};

class OPIfStmtContext: public OPContext {
public:
    OPIfStmtContext() : OPContext(ContextKind_IfStmt) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_IfStmt;
    }
    
public:
    OPContext *m_condContext = NULL;
    OPContext *m_compContext = NULL;
    OPContext *m_elseCompContext = NULL;
    bool m_hasElse;
    
    string parse() {
        string script = "";
        
        script += "if (";
        script += m_condContext->parse();
        script += ") {\n";
        script += m_compContext->parse();
        script += "}\n";
        if (m_elseCompContext) {
            script += "else {\n";
            script += m_elseCompContext->parse();
            script += "}\n";
        } else if (m_hasElse) {
            script += "else ";
        }
        return script;
    }
};

// string
class OPStringLiteralContext: public OPContext {
public:
    OPStringLiteralContext() : OPContext(ContextKind_StringLiteral) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_StringLiteral;
    }
    
public:
    string value;
    
    string parse() {
        string script = "";
        
        script += value;
        
        return script;
    }
};

// 345
class OPIntegerLiteralContext: public OPContext {
public:
    OPIntegerLiteralContext() : OPContext(ContextKind_IntegerLiteral) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_IntegerLiteral;
    }
public:
    int64_t value;
    
    string parse() {
        string script = "";
        
        script += std::to_string(value);
        
        return script;
    }
};

// 3.45
class OPFloatingLiteralContext: public OPContext {
public:
    OPFloatingLiteralContext() : OPContext(ContextKind_FloatingLiteral) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_FloatingLiteral;
    }
public:
    float value;
    
    string parse() {
        string script = ",";
        
        script += std::to_string(value);
        
        return script;
    }
};

// a()
class OPCallExprContext: public OPContext {
public:
    OPCallExprContext() : OPContext(ContextKind_CallExpr) {}
    static bool classof(const OPContext *S) {
        return S->getKind() == ContextKind_CallExpr;
    }
public:
    string m_funcName;
    vector<OPParam> m_fixParams;
    vector<OPParam> m_varParams;
    string returnType;
    
    string parse() {
        string script = "callCFunction('";

        script += m_funcName;
        script += "',";
        
        if (m_fixParams.size() > 0) {
            for (vector<OPParam>::iterator itr = m_varParams.begin(); itr != m_varParams.end(); itr++) {
                script += ",";
                OPParam param = *itr;
                //                script += param.value;
            }
            
            
            OPContext *nextCtx = m_next;
            while (nextCtx) {
                script += nextCtx->parse();
                nextCtx = nextCtx->m_next;
            }
        }
        
        script += "'";
        script += returnType;
        script += "'";
        script += ");";
        
        return script;
    }
};

#endif /* OPContext_hpp */
