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

using namespace std;

class OPParam
{
public:
    char *type;
    void *value;
};

class OPContext {
public:
    OPContext *m_next = NULL;
    OPContext *m_pre = NULL;
    int m_currIdx;
    
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

class OPIfStmtContext: public OPContext {
public:
    OPContext *m_condContext = NULL;
    OPIfStmtContext *m_elseContext;
    
    string parse() {
        string script = "";
        
        script += "if (";
        script += m_condContext->parse();
        script += ") {";
        
//        script += "}";
        return script;
    }
};

// string
class OPStringLiteralContext: public OPContext {
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
