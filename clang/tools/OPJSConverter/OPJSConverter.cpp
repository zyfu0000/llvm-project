#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <iostream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "OPContext.hpp"
#include <regex>
#include <typeinfo>

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

#define OPCheckContextInvalid(x) if (x->getBeginLoc().isInvalid() || x->getEndLoc().isInvalid()) { \
return;\
}

#define OPCheckInvalid(x) if (x->getBeginLoc().isInvalid() || x->getEndLoc().isInvalid()) { \
return true;\
}

static cl::OptionCategory OPOptionCategory("OPOptionCategory");

class OPASTVisitor : public RecursiveASTVisitor<OPASTVisitor>
{
public:
    explicit OPASTVisitor(CompilerInstance *aCI) : m_compilerInstance(aCI) {}
    
    bool dataTraverseStmtPost(Stmt *S)
    {
        if (!m_currentContext) {
            return true;
        }
        
        if (!isUserSourceDecl(S)) {
            return true;
        }
        
        if (isa<CompoundStmt>(S)) {
            OPContext *preContext = m_currentContext;
            while (!preContext->m_parent) {
                preContext = preContext->m_pre;
                if (!preContext) {
                    return true;
                }
            }
            m_currentContext = preContext->m_parent;
            
            return true;
        }
        return true;
    }
    
    // @implementation xx
    bool VisitObjCImplementationDecl(const ObjCImplementationDecl *implDecl)
    {
        OPCheckInvalid(implDecl)
        
        OPImplementationDeclContext *context = new OPImplementationDeclContext;
        context->m_className = implDecl->getNameAsString();
        m_classContexts.push_back(context);
        
        m_currentImplContext = context;
        
        return true;
    }
    
    // - (void)xxx{}
    bool VisitObjCMethodDecl(const ObjCMethodDecl *methodDecl)
    {
        OPCheckInvalid(methodDecl)
        
        // annotation 标注 patch 版本
        if (methodDecl->hasAttr<AnnotateAttr>()) {
            AnnotateAttr *attr = methodDecl->getAttr<AnnotateAttr>();
            cout << "标注：" << attr->getAttrName() << endl;
        }
        
        //隐式实现的，并非在显示的写源码中则不作处理。例如：编译器会实现 property 的getter 和 setter 方法
        if (methodDecl->isImplicit()) {
            return false;
        }
        
        // 获取返回类型
        QualType qualType = methodDecl->getReturnType();
        if (qualType.isNull()) {
            return false;
        }
        TypeSourceInfo *typeSourceInfo = methodDecl->getReturnTypeSourceInfo();
        if (!typeSourceInfo) {
            return false;
        }
        
        if (!m_currentImplContext) {
            return true;
        }
        
        OPMethodDeclContext *context = new OPMethodDeclContext;
        context->m_className = methodDecl->getClassInterface()->getNameAsString();
        std::string methodName = methodDecl->getNameAsString();
        convertToOPJSMethod(methodName);
        context->m_methodName = methodName;
        context->m_isClassMethod = methodDecl->isClassMethod();
        m_currentImplContext->m_methodContexts.push_back(context);
        m_currentContext = context;

        // 获取参数列表
        ArrayRef<ParmVarDecl*> params = methodDecl->parameters();
        // 遍历参数列表
        for (ArrayRef<ParmVarDecl *>::iterator i = params.begin(), e = params.end(); i != e; i++) {
            ParmVarDecl *p = *i;
            string name = p->getQualifiedNameAsString();
            context->m_params.push_back(name);
        }
        
        return true;
    }
    
    // id a = xx;
    bool VisitVarDecl(VarDecl *varDecl)
    {
        return true;
    }
    
    // [self a]
    bool VisitObjCMessageExpr(ObjCMessageExpr* messageExpr)
    {
        OPCheckInvalid(messageExpr)
        
        if (!m_currentContext) {
            return true;
        }
        
        if (std::find(m_parsedExpr.begin(), m_parsedExpr.end(), messageExpr) != m_parsedExpr.end()) {
            return true;
        }
        
        OPMessageExprContext *context = (OPMessageExprContext *)convertExprToOpContext(messageExpr);
        
        m_currentContext->setNext(context);
        m_currentContext = context;
        
        return true;
    }
    
    OPContext *convertExprToOpContext(Expr *expr)
    {
        if (isa<ObjCMessageExpr>(expr)) {
            OPMessageExprContext *context = new OPMessageExprContext;
            ObjCMessageExpr *messageExpr = (ObjCMessageExpr *)expr;
            if (messageExpr->isClassMessage()) {
                context->m_isClassMethod = messageExpr->isClassMessage();
                
                const ObjCInterfaceDecl *objcInterfaceDecl = messageExpr->getReceiverInterface();
                string className = objcInterfaceDecl->getNameAsString();
                context->m_className = className;
            } else {
                Expr *receiverExpr = messageExpr->getInstanceReceiver();
                context->m_receiverContext = convertExprToOpContext(receiverExpr);
            }
            std::string selName = messageExpr->getSelector().getAsString();
            convertToOPJSMethod(selName);
            context->m_selName = selName;

            uint16_t argsNum = messageExpr->getNumArgs();
            for (uint16_t i = 0; i < argsNum; ++i) {
                const Expr *arg = messageExpr->getArg(i);
                arg->getType();
            }
            
            m_parsedExpr.push_back(expr);
            
            return context;
        } else if (isa<ImplicitCastExpr>(expr)) {
            ImplicitCastExpr *castExpr = (ImplicitCastExpr *)expr;
            return convertExprToOpContext(castExpr->getSubExpr());
        } else if (isa<DeclRefExpr>(expr)) {
            DeclRefExpr *refExpr = (DeclRefExpr *)expr;
            OPStringLiteralContext *context = new OPStringLiteralContext;
            context->value = refExpr->getNameInfo().getName().getAsString();
            
            return context;
        }
        
        return NULL;
    }
    
    bool VisitDeclStmt(DeclStmt *stmt) {
        OPCheckInvalid(stmt)

        if (!m_currentContext) {
            return true;
        }
        
        if (!stmt->isSingleDecl()) {
            return true;
        }
        
        VarDecl *varDecl = (VarDecl *)stmt->getSingleDecl();
        
        if (!varDecl->isLocalVarDecl()) {
            return true;
        }

        OPDeclStmtContext *context = new OPDeclStmtContext;
        context->m_varName = varDecl->getNameAsString();
        context->m_initContext = convertExprToOpContext(varDecl->getInit());

        m_currentContext->setNext(context);
        m_currentContext = context;

        return true;
    }
    
    // if () {}
    bool VisitIfStmt(IfStmt* stmt)
    {
        OPCheckInvalid(stmt)
        
        if (!isUserSourceDecl(stmt)) {
            return true;
        }
        
        OPIfStmtContext *context = new OPIfStmtContext;
        context->m_condContext = convertExprToOpContext(stmt->getCond());
        context->m_hasElse = stmt->hasElseStorage();
        
        m_currentContext->setNext(context);
        m_currentContext = context;
        
        return true;
    }
    
    bool VisitCompoundStmt(CompoundStmt *stmt)
    {
        OPCheckInvalid(stmt)
        
        if (!isUserSourceDecl(stmt)) {
            return true;
        }
        
        if (!m_currentContext) {
            return true;
        }
        
        if (isa<OPIfStmtContext>(m_currentContext)) {
            OPCompoundStmtContext *context = new OPCompoundStmtContext;
            context->m_parent = m_currentContext;
            
            OPIfStmtContext *currentContext = (OPIfStmtContext *)m_currentContext;
            if (currentContext->m_compContext) {
                currentContext->m_elseCompContext = context;
            } else {
                currentContext->m_compContext = context;
            }
            
            m_currentContext = context;
        }
        return true;
    }
    
    // switch case
    bool VisitSwitchStmt(SwitchStmt *stmt)
    {
        return true;
    }
    
    // white () {}
    bool VisitWhileStmt(WhileStmt *stmt)
    {
        return true;
    }
    
    // do {} while()
    bool VisitDoStmt(DoStmt *stmt)
    {
        return true;
    }
    
    bool VisitObjCSelectorExpr(ObjCSelectorExpr *expr)
    {
        return true;
    }
    
    // void a(){}
    bool VisitFunctionDecl(FunctionDecl *functionDecl)
    {
        //ParmVarDecl
        
        return true;
    }
    
    // a()
    bool VisitCallExpr(CallExpr *callExpt)
    {
        return true;
    }
    
    bool VisitDeclRefExpr(DeclRefExpr *declExpt)
    {
        return true;
    }
    
    // 345
    bool VisitIntegerLiteral(IntegerLiteral *literal)
    {
        OPCheckInvalid(literal)
        
        if (!m_currentContext) {
            return true;
        }
        
        OPIntegerLiteralContext *context = new OPIntegerLiteralContext;
        context->value = literal->getValue().getLimitedValue();
        
        m_currentContext->setNext(context);
        m_currentContext = context;
        
        return true;
    }
    
    // 4.56
    bool VisitFloatingLiteral(FloatingLiteral *literal)
    {
        OPCheckInvalid(literal)
        
        if (!m_currentContext) {
            return true;
        }
        
        OPFloatingLiteralContext *context = new OPFloatingLiteralContext;
        context->value = literal->getValue().convertToFloat();
        
        m_currentContext->setNext(context);
        m_currentContext = context;
        
        return true;
    }
    
    // "xxx"
    bool VisitStringLiteral(clang::StringLiteral *literal)
    {
        return true;
    }
    
    // @""
    bool VisitObjCStringLiteral(ObjCStringLiteral *literal)
    {
        return true;
    }
    
    // class/union/struct AA
    bool VisitRecordDecl(RecordDecl *decl)
    {
        // FieldDecl
        
        return true;
    }
    
    void convertToOPJSMethod(std::string &methodName)
    {
        if (methodName.back() == ':') {
            methodName.pop_back();
        }
        methodName = std::regex_replace(methodName, std::regex("_"), "__");
        methodName = std::regex_replace(methodName, std::regex(":"), "_");
    }
    
    
    //判断是否用户源码，过滤掉系统源码
    template <typename Node>
    bool isUserSourceDecl(const Node node) {
        if (!node) {
            return false;
        }
        string filename = sourcePathNode(node);
        if (filename.empty()) {
            return false;
        }
        //非XCode中的源码都认为是用户源码
        if (filename.find("/Applications/Xcode.app/") == 0) {
            return false;
        }
        return true;
    }
    
    //获取decl所在的文件
    template <typename Node>
    string sourcePathNode(const Node node ) {
        if(!node)return "";
        
        SourceLocation spellingLoc = m_compilerInstance->getSourceManager().getSpellingLoc(node->getSourceRange().getBegin());
        string filePath = m_compilerInstance->getSourceManager().getFilename(spellingLoc).str();
        return filePath;
    }
    
    string script() {
        string text;
        for (vector<OPImplementationDeclContext *>::iterator i = m_classContexts.begin(), e = m_classContexts.end(); i != e; i++) {
            OPImplementationDeclContext *p = *i;
            
            text += p->parse();
        }
        
        return text;
    }
private:
    CompilerInstance *m_compilerInstance;

    vector<OPImplementationDeclContext *> m_classContexts;
    OPImplementationDeclContext *m_currentImplContext = NULL;
    OPContext *m_currentContext = NULL;
    
    vector<Stmt *> m_parsedExpr;
};

//AST 构造器
class OPASTConsumer : public ASTConsumer {
public:
    explicit OPASTConsumer(CompilerInstance *aCI) : Visitor(aCI) {}
    
    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
    
    string script() {
        return Visitor.script();
    }
private:
    OPASTVisitor Visitor;
    
};

//action
class OPASTFrontendAction : public ASTFrontendAction {
public:
    //创建AST Consumer
    std::unique_ptr<ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, StringRef file) override {
        size_t index = file.str().find_last_of("/");
        StringRef fileName = file.str().substr(index+1,-1); //获取文件名，截取'/'后面的部分
        cout << "【混淆】开始处理文件：" << fileName.str()  <<  endl;
        
        this->consumer = new OPASTConsumer(&CI);
        return std::unique_ptr<clang::ASTConsumer>(this->consumer);
    }
    //源文件操作结束
    void EndSourceFileAction() override {
        //文件处理完成
        cout << "【混淆】文件处理完成: " << this->consumer->script() << endl;
    }
    
private:
    OPASTConsumer *consumer;
};

int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, OPOptionCategory);
    ClangTool Tool(OptionsParser.getCompilations(),OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<OPASTFrontendAction>().get());
}
