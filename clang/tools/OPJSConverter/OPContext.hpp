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
class OPMethodDeclContext: public OPContext {
public:
    string m_className;
    string m_methodName;
    vector<string> m_params;
    
    string parse() {
        string script = "fixMethod(";
        
        script = script + "'" + m_className + "',";
        script = script + "'" + m_methodName + "',";
        script += "false,";
        script += "function (self, sel";
        if (m_params.size() > 0) {
            for (vector<string>::iterator itr = m_params.begin(); itr != m_params.end(); itr++) {
                script += ",";
                script += *itr;
            }
        }
        script += "){ \n";
        
        OPContext *nextCtx = m_next;
        if (nextCtx) {
            script += nextCtx->parse();
//            nextCtx = nextCtx->m_next;
        }
        
        script += "}); \n";
        
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
        
        script = script + m_className + "{ \n";
        
        for (vector<OPMethodDeclContext *>::iterator i = m_methodContexts.begin(), e = m_methodContexts.end(); i != e; i++) {
            OPMethodDeclContext *p = *i;
            
            script += p->parse();
            script += "\n";
        }
        
        script += "} \n";
        
        return script;
    }
};

// [self a]
class OPMessageExprContext: public OPContext {
public:
    bool m_isStatic;
    string m_classOrInstanceName;
    string m_selName;
    vector<OPParam> m_params;
//    returnType;
    
    string parse() {
        string script = "";
        
        if (m_isStatic) {
            script += "callClassMethod('";
            script += m_classOrInstanceName;
            script += "',";
        } else {
            script += "callInstanceMethod(";
            script += m_classOrInstanceName;
            script += ",";
        }
        script += "'";
        script += m_selName;
        script += "'";
        
        if (m_params.size() > 0) {
            for (vector<OPParam>::iterator itr = m_params.begin(); itr != m_params.end(); itr++) {
                script += ",";
                OPParam param = *itr;
//                script += param.value;
            }
        }
        
        
        OPContext *nextCtx = m_next;
        while (nextCtx) {
            script += nextCtx->parse();
            nextCtx = nextCtx->m_next;
        }
        
        script += ");";
        
        return script;
    }
};

// 345
class OPIntegerLiteralContext: public OPContext {
public:
    int64_t value;
    
    string parse() {
        string script = ",";
        
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
