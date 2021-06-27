class VarDecl : public DeclaratorDecl, public Redeclarable<VarDecl> {
public:
  /// Initialization styles.
  enum InitializationStyle {
    /// C-style initialization with assignment
    CInit,

    /// Call-style initialization (C++98)
    CallInit,

    /// Direct list-initialization (C++11)
    ListInit
  };

  /// Kinds of thread-local storage.
  enum TLSKind {
    /// Not a TLS variable.
    TLS_None,

    /// TLS with a known-constant initializer.
    TLS_Static,

    /// TLS with a dynamic initializer.
    TLS_Dynamic
  };

  /// Return the string used to specify the storage class \p SC.
  ///
  /// It is illegal to call this function with SC == None.
  static const char *getStorageClassSpecifierString(StorageClass SC);

protected:
  // A pointer union of Stmt * and EvaluatedStmt *. When an EvaluatedStmt, we
  // have allocated the auxiliary struct of information there.
  //
  // TODO: It is a bit unfortunate to use a PointerUnion inside the VarDecl for
  // this as *many* VarDecls are ParmVarDecls that don't have default
  // arguments. We could save some space by moving this pointer union to be
  // allocated in trailing space when necessary.
  using InitType = llvm::PointerUnion<Stmt *, EvaluatedStmt *>;

  /// The initializer for this variable or, for a ParmVarDecl, the
  /// C++ default argument.
  mutable InitType Init;

private:
  friend class ASTDeclReader;
  friend class ASTNodeImporter;
  friend class StmtIteratorBase;

  class VarDeclBitfields {
    friend class ASTDeclReader;
    friend class VarDecl;

    unsigned SClass : 3;
    unsigned TSCSpec : 2;
    unsigned InitStyle : 2;

    /// Whether this variable is an ARC pseudo-__strong variable; see
    /// isARCPseudoStrong() for details.
    unsigned ARCPseudoStrong : 1;
  };
  enum { NumVarDeclBits = 8 };

protected:
  enum { NumParameterIndexBits = 8 };

  enum DefaultArgKind {
    DAK_None,
    DAK_Unparsed,
    DAK_Uninstantiated,
    DAK_Normal
  };

  enum { NumScopeDepthOrObjCQualsBits = 7 };

  class ParmVarDeclBitfields {
    friend class ASTDeclReader;
    friend class ParmVarDecl;

    unsigned : NumVarDeclBits;

    /// Whether this parameter inherits a default argument from a
    /// prior declaration.
    unsigned HasInheritedDefaultArg : 1;

    /// Describes the kind of default argument for this parameter. By default
    /// this is none. If this is normal, then the default argument is stored in
    /// the \c VarDecl initializer expression unless we were unable to parse
    /// (even an invalid) expression for the default argument.
    unsigned DefaultArgKind : 2;

    /// Whether this parameter undergoes K&R argument promotion.
    unsigned IsKNRPromoted : 1;

    /// Whether this parameter is an ObjC method parameter or not.
    unsigned IsObjCMethodParam : 1;

    /// If IsObjCMethodParam, a Decl::ObjCDeclQualifier.
    /// Otherwise, the number of function parameter scopes enclosing
    /// the function parameter scope in which this parameter was
    /// declared.
    unsigned ScopeDepthOrObjCQuals : NumScopeDepthOrObjCQualsBits;

    /// The number of parameters preceding this parameter in the
    /// function parameter scope in which it was declared.
    unsigned ParameterIndex : NumParameterIndexBits;
  };

  class NonParmVarDeclBitfields {
    friend class ASTDeclReader;
    friend class ImplicitParamDecl;
    friend class VarDecl;

    unsigned : NumVarDeclBits;

    // FIXME: We need something similar to CXXRecordDecl::DefinitionData.
    /// Whether this variable is a definition which was demoted due to
    /// module merge.
    unsigned IsThisDeclarationADemotedDefinition : 1;

    /// Whether this variable is the exception variable in a C++ catch
    /// or an Objective-C @catch statement.
    unsigned ExceptionVar : 1;

    /// Whether this local variable could be allocated in the return
    /// slot of its function, enabling the named return value optimization
    /// (NRVO).
    unsigned NRVOVariable : 1;

    /// Whether this variable is the for-range-declaration in a C++0x
    /// for-range statement.
    unsigned CXXForRangeDecl : 1;

    /// Whether this variable is the for-in loop declaration in Objective-C.
    unsigned ObjCForDecl : 1;

    /// Whether this variable is (C++1z) inline.
    unsigned IsInline : 1;

    /// Whether this variable has (C++1z) inline explicitly specified.
    unsigned IsInlineSpecified : 1;

    /// Whether this variable is (C++0x) constexpr.
    unsigned IsConstexpr : 1;

    /// Whether this variable is the implicit variable for a lambda
    /// init-capture.
    unsigned IsInitCapture : 1;

    /// Whether this local extern variable's previous declaration was
    /// declared in the same block scope. This controls whether we should merge
    /// the type of this declaration with its previous declaration.
    unsigned PreviousDeclInSameBlockScope : 1;

    /// Defines kind of the ImplicitParamDecl: 'this', 'self', 'vtt', '_cmd' or
    /// something else.
    unsigned ImplicitParamKind : 3;

    unsigned EscapingByref : 1;
  };

  union {
    unsigned AllBits;
    VarDeclBitfields VarDeclBits;
    ParmVarDeclBitfields ParmVarDeclBits;
    NonParmVarDeclBitfields NonParmVarDeclBits;
  };

  VarDecl(Kind DK, ASTContext &C, DeclContext *DC, SourceLocation StartLoc,
          SourceLocation IdLoc, IdentifierInfo *Id, QualType T,
          TypeSourceInfo *TInfo, StorageClass SC);

  using redeclarable_base = Redeclarable<VarDecl>;

  VarDecl *getNextRedeclarationImpl() override {
    return getNextRedeclaration();
  }

  VarDecl *getPreviousDeclImpl() override {
    return getPreviousDecl();
  }

  VarDecl *getMostRecentDeclImpl() override {
    return getMostRecentDecl();
  }

public:
  using redecl_range = redeclarable_base::redecl_range;
  using redecl_iterator = redeclarable_base::redecl_iterator;

  using redeclarable_base::redecls_begin;
  using redeclarable_base::redecls_end;
  using redeclarable_base::redecls;
  using redeclarable_base::getPreviousDecl;
  using redeclarable_base::getMostRecentDecl;
  using redeclarable_base::isFirstDecl;

  static VarDecl *Create(ASTContext &C, DeclContext *DC,
                         SourceLocation StartLoc, SourceLocation IdLoc,
                         IdentifierInfo *Id, QualType T, TypeSourceInfo *TInfo,
                         StorageClass S);

  static VarDecl *CreateDeserialized(ASTContext &C, unsigned ID);

  SourceRange getSourceRange() const override LLVM_READONLY;

  /// Returns the storage class as written in the source. For the
  /// computed linkage of symbol, see getLinkage.
  StorageClass getStorageClass() const {
    return (StorageClass) VarDeclBits.SClass;
  }
  void setStorageClass(StorageClass SC);

  void setTSCSpec(ThreadStorageClassSpecifier TSC) {
    VarDeclBits.TSCSpec = TSC;
    assert(VarDeclBits.TSCSpec == TSC && "truncation");
  }
  ThreadStorageClassSpecifier getTSCSpec() const {
    return static_cast<ThreadStorageClassSpecifier>(VarDeclBits.TSCSpec);
  }
  TLSKind getTLSKind() const;

  /// Returns true if a variable with function scope is a non-static local
  /// variable.
  bool hasLocalStorage() const {
    if (getStorageClass() == SC_None) {
      // OpenCL v1.2 s6.5.3: The __constant or constant address space name is
      // used to describe variables allocated in global memory and which are
      // accessed inside a kernel(s) as read-only variables. As such, variables
      // in constant address space cannot have local storage.
      if (getType().getAddressSpace() == LangAS::opencl_constant)
        return false;
      // Second check is for C++11 [dcl.stc]p4.
      return !isFileVarDecl() && getTSCSpec() == TSCS_unspecified;
    }

    // Global Named Register (GNU extension)
    if (getStorageClass() == SC_Register && !isLocalVarDeclOrParm())
      return false;

    // Return true for:  Auto, Register.
    // Return false for: Extern, Static, PrivateExtern, OpenCLWorkGroupLocal.

    return getStorageClass() >= SC_Auto;
  }

  /// Returns true if a variable with function scope is a static local
  /// variable.
  bool isStaticLocal() const {
    return (getStorageClass() == SC_Static ||
            // C++11 [dcl.stc]p4
            (getStorageClass() == SC_None && getTSCSpec() == TSCS_thread_local))
      && !isFileVarDecl();
  }

  /// Returns true if a variable has extern or __private_extern__
  /// storage.
  bool hasExternalStorage() const {
    return getStorageClass() == SC_Extern ||
           getStorageClass() == SC_PrivateExtern;
  }

  /// Returns true for all variables that do not have local storage.
  ///
  /// This includes all global variables as well as static variables declared
  /// within a function.
  bool hasGlobalStorage() const { return !hasLocalStorage(); }

  /// Get the storage duration of this variable, per C++ [basic.stc].
  StorageDuration getStorageDuration() const {
    return hasLocalStorage() ? SD_Automatic :
           getTSCSpec() ? SD_Thread : SD_Static;
  }

  /// Compute the language linkage.
  LanguageLinkage getLanguageLinkage() const;

  /// Determines whether this variable is a variable with external, C linkage.
  bool isExternC() const;

  /// Determines whether this variable's context is, or is nested within,
  /// a C++ extern "C" linkage spec.
  bool isInExternCContext() const;

  /// Determines whether this variable's context is, or is nested within,
  /// a C++ extern "C++" linkage spec.
  bool isInExternCXXContext() const;

  /// Returns true for local variable declarations other than parameters.
  /// Note that this includes static variables inside of functions. It also
  /// includes variables inside blocks.
  ///
  ///   void foo() { int x; static int y; extern int z; }
  bool isLocalVarDecl() const {
    if (getKind() != Decl::Var && getKind() != Decl::Decomposition)
      return false;
    if (const DeclContext *DC = getLexicalDeclContext())
      return DC->getRedeclContext()->isFunctionOrMethod();
    return false;
  }

  /// Similar to isLocalVarDecl but also includes parameters.
  bool isLocalVarDeclOrParm() const {
    return isLocalVarDecl() || getKind() == Decl::ParmVar;
  }

  /// Similar to isLocalVarDecl, but excludes variables declared in blocks.
  bool isFunctionOrMethodVarDecl() const {
    if (getKind() != Decl::Var && getKind() != Decl::Decomposition)
      return false;
    const DeclContext *DC = getLexicalDeclContext()->getRedeclContext();
    return DC->isFunctionOrMethod() && DC->getDeclKind() != Decl::Block;
  }

  /// Determines whether this is a static data member.
  ///
  /// This will only be true in C++, and applies to, e.g., the
  /// variable 'x' in:
  /// \code
  /// struct S {
  ///   static int x;
  /// };
  /// \endcode
  bool isStaticDataMember() const {
    // If it wasn't static, it would be a FieldDecl.
    return getKind() != Decl::ParmVar && getDeclContext()->isRecord();
  }

  VarDecl *getCanonicalDecl() override;
  const VarDecl *getCanonicalDecl() const {
    return const_cast<VarDecl*>(this)->getCanonicalDecl();
  }

  enum DefinitionKind {
    /// This declaration is only a declaration.
    DeclarationOnly,

    /// This declaration is a tentative definition.
    TentativeDefinition,

    /// This declaration is definitely a definition.
    Definition
  };

  /// Check whether this declaration is a definition. If this could be
  /// a tentative definition (in C), don't check whether there's an overriding
  /// definition.
  DefinitionKind isThisDeclarationADefinition(ASTContext &) const;
  DefinitionKind isThisDeclarationADefinition() const {
    return isThisDeclarationADefinition(getASTContext());
  }

  /// Check whether this variable is defined in this translation unit.
  DefinitionKind hasDefinition(ASTContext &) const;
  DefinitionKind hasDefinition() const {
    return hasDefinition(getASTContext());
  }

  /// Get the tentative definition that acts as the real definition in a TU.
  /// Returns null if there is a proper definition available.
  VarDecl *getActingDefinition();
  const VarDecl *getActingDefinition() const {
    return const_cast<VarDecl*>(this)->getActingDefinition();
  }

  /// Get the real (not just tentative) definition for this declaration.
  VarDecl *getDefinition(ASTContext &);
  const VarDecl *getDefinition(ASTContext &C) const {
    return const_cast<VarDecl*>(this)->getDefinition(C);
  }
  VarDecl *getDefinition() {
    return getDefinition(getASTContext());
  }
  const VarDecl *getDefinition() const {
    return const_cast<VarDecl*>(this)->getDefinition();
  }

  /// Determine whether this is or was instantiated from an out-of-line
  /// definition of a static data member.
  bool isOutOfLine() const override;

  /// Returns true for file scoped variable declaration.
  bool isFileVarDecl() const {
    Kind K = getKind();
    if (K == ParmVar || K == ImplicitParam)
      return false;

    if (getLexicalDeclContext()->getRedeclContext()->isFileContext())
      return true;

    if (isStaticDataMember())
      return true;

    return false;
  }

  /// Get the initializer for this variable, no matter which
  /// declaration it is attached to.
  const Expr *getAnyInitializer() const {
    const VarDecl *D;
    return getAnyInitializer(D);
  }

  /// Get the initializer for this variable, no matter which
  /// declaration it is attached to. Also get that declaration.
  const Expr *getAnyInitializer(const VarDecl *&D) const;

  bool hasInit() const;
  const Expr *getInit() const {
    return const_cast<VarDecl *>(this)->getInit();
  }
  Expr *getInit();

  /// Retrieve the address of the initializer expression.
  Stmt **getInitAddress();

  void setInit(Expr *I);

  /// Get the initializing declaration of this variable, if any. This is
  /// usually the definition, except that for a static data member it can be
  /// the in-class declaration.
  VarDecl *getInitializingDeclaration();
  const VarDecl *getInitializingDeclaration() const {
    return const_cast<VarDecl *>(this)->getInitializingDeclaration();
  }

  /// Determine whether this variable's value might be usable in a
  /// constant expression, according to the relevant language standard.
  /// This only checks properties of the declaration, and does not check
  /// whether the initializer is in fact a constant expression.
  ///
  /// This corresponds to C++20 [expr.const]p3's notion of a
  /// "potentially-constant" variable.
  bool mightBeUsableInConstantExpressions(const ASTContext &C) const;

  /// Determine whether this variable's value can be used in a
  /// constant expression, according to the relevant language standard,
  /// including checking whether it was initialized by a constant expression.
  bool isUsableInConstantExpressions(const ASTContext &C) const;

  EvaluatedStmt *ensureEvaluatedStmt() const;
  EvaluatedStmt *getEvaluatedStmt() const;

  /// Attempt to evaluate the value of the initializer attached to this
  /// declaration, and produce notes explaining why it cannot be evaluated.
  /// Returns a pointer to the value if evaluation succeeded, 0 otherwise.
  APValue *evaluateValue() const;

private:
  APValue *evaluateValueImpl(SmallVectorImpl<PartialDiagnosticAt> &Notes,
                             bool IsConstantInitialization) const;

public:
  /// Return the already-evaluated value of this variable's
  /// initializer, or NULL if the value is not yet known. Returns pointer
  /// to untyped APValue if the value could not be evaluated.
  APValue *getEvaluatedValue() const;

  /// Evaluate the destruction of this variable to determine if it constitutes
  /// constant destruction.
  ///
  /// \pre hasConstantInitialization()
  /// \return \c true if this variable has constant destruction, \c false if
  ///         not.
  bool evaluateDestruction(SmallVectorImpl<PartialDiagnosticAt> &Notes) const;

  /// Determine whether this variable has constant initialization.
  ///
  /// This is only set in two cases: when the language semantics require
  /// constant initialization (globals in C and some globals in C++), and when
  /// the variable is usable in constant expressions (constexpr, const int, and
  /// reference variables in C++).
  bool hasConstantInitialization() const;

  /// Determine whether the initializer of this variable is an integer constant
  /// expression. For use in C++98, where this affects whether the variable is
  /// usable in constant expressions.
  bool hasICEInitializer(const ASTContext &Context) const;

  /// Evaluate the initializer of this variable to determine whether it's a
  /// constant initializer. Should only be called once, after completing the
  /// definition of the variable.
  bool checkForConstantInitialization(
      SmallVectorImpl<PartialDiagnosticAt> &Notes) const;

  void setInitStyle(InitializationStyle Style) {
    VarDeclBits.InitStyle = Style;
  }

  /// The style of initialization for this declaration.
  ///
  /// C-style initialization is "int x = 1;". Call-style initialization is
  /// a C++98 direct-initializer, e.g. "int x(1);". The Init expression will be
  /// the expression inside the parens or a "ClassType(a,b,c)" class constructor
  /// expression for class types. List-style initialization is C++11 syntax,
  /// e.g. "int x{1};". Clients can distinguish between different forms of
  /// initialization by checking this value. In particular, "int x = {1};" is
  /// C-style, "int x({1})" is call-style, and "int x{1};" is list-style; the
  /// Init expression in all three cases is an InitListExpr.
  InitializationStyle getInitStyle() const {
    return static_cast<InitializationStyle>(VarDeclBits.InitStyle);
  }

  /// Whether the initializer is a direct-initializer (list or call).
  bool isDirectInit() const {
    return getInitStyle() != CInit;
  }

  /// If this definition should pretend to be a declaration.
  bool isThisDeclarationADemotedDefinition() const {
    return isa<ParmVarDecl>(this) ? false :
      NonParmVarDeclBits.IsThisDeclarationADemotedDefinition;
  }

  /// This is a definition which should be demoted to a declaration.
  ///
  /// In some cases (mostly module merging) we can end up with two visible
  /// definitions one of which needs to be demoted to a declaration to keep
  /// the AST invariants.
  void demoteThisDefinitionToDeclaration() {
    assert(isThisDeclarationADefinition() && "Not a definition!");
    assert(!isa<ParmVarDecl>(this) && "Cannot demote ParmVarDecls!");
    NonParmVarDeclBits.IsThisDeclarationADemotedDefinition = 1;
  }

  /// Determine whether this variable is the exception variable in a
  /// C++ catch statememt or an Objective-C \@catch statement.
  bool isExceptionVariable() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.ExceptionVar;
  }
  void setExceptionVariable(bool EV) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.ExceptionVar = EV;
  }

  /// Determine whether this local variable can be used with the named
  /// return value optimization (NRVO).
  ///
  /// The named return value optimization (NRVO) works by marking certain
  /// non-volatile local variables of class type as NRVO objects. These
  /// locals can be allocated within the return slot of their containing
  /// function, in which case there is no need to copy the object to the
  /// return slot when returning from the function. Within the function body,
  /// each return that returns the NRVO object will have this variable as its
  /// NRVO candidate.
  bool isNRVOVariable() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.NRVOVariable;
  }
  void setNRVOVariable(bool NRVO) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.NRVOVariable = NRVO;
  }

  /// Determine whether this variable is the for-range-declaration in
  /// a C++0x for-range statement.
  bool isCXXForRangeDecl() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.CXXForRangeDecl;
  }
  void setCXXForRangeDecl(bool FRD) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.CXXForRangeDecl = FRD;
  }

  /// Determine whether this variable is a for-loop declaration for a
  /// for-in statement in Objective-C.
  bool isObjCForDecl() const {
    return NonParmVarDeclBits.ObjCForDecl;
  }

  void setObjCForDecl(bool FRD) {
    NonParmVarDeclBits.ObjCForDecl = FRD;
  }

  /// Determine whether this variable is an ARC pseudo-__strong variable. A
  /// pseudo-__strong variable has a __strong-qualified type but does not
  /// actually retain the object written into it. Generally such variables are
  /// also 'const' for safety. There are 3 cases where this will be set, 1) if
  /// the variable is annotated with the objc_externally_retained attribute, 2)
  /// if its 'self' in a non-init method, or 3) if its the variable in an for-in
  /// loop.
  bool isARCPseudoStrong() const { return VarDeclBits.ARCPseudoStrong; }
  void setARCPseudoStrong(bool PS) { VarDeclBits.ARCPseudoStrong = PS; }

  /// Whether this variable is (C++1z) inline.
  bool isInline() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.IsInline;
  }
  bool isInlineSpecified() const {
    return isa<ParmVarDecl>(this) ? false
                                  : NonParmVarDeclBits.IsInlineSpecified;
  }
  void setInlineSpecified() {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.IsInline = true;
    NonParmVarDeclBits.IsInlineSpecified = true;
  }
  void setImplicitlyInline() {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.IsInline = true;
  }

  /// Whether this variable is (C++11) constexpr.
  bool isConstexpr() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.IsConstexpr;
  }
  void setConstexpr(bool IC) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.IsConstexpr = IC;
  }

  /// Whether this variable is the implicit variable for a lambda init-capture.
  bool isInitCapture() const {
    return isa<ParmVarDecl>(this) ? false : NonParmVarDeclBits.IsInitCapture;
  }
  void setInitCapture(bool IC) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.IsInitCapture = IC;
  }

  /// Determine whether this variable is actually a function parameter pack or
  /// init-capture pack.
  bool isParameterPack() const;

  /// Whether this local extern variable declaration's previous declaration
  /// was declared in the same block scope. Only correct in C++.
  bool isPreviousDeclInSameBlockScope() const {
    return isa<ParmVarDecl>(this)
               ? false
               : NonParmVarDeclBits.PreviousDeclInSameBlockScope;
  }
  void setPreviousDeclInSameBlockScope(bool Same) {
    assert(!isa<ParmVarDecl>(this));
    NonParmVarDeclBits.PreviousDeclInSameBlockScope = Same;
  }

  /// Indicates the capture is a __block variable that is captured by a block
  /// that can potentially escape (a block for which BlockDecl::doesNotEscape
  /// returns false).
  bool isEscapingByref() const;

  /// Indicates the capture is a __block variable that is never captured by an
  /// escaping block.
  bool isNonEscapingByref() const;

  void setEscapingByref() {
    NonParmVarDeclBits.EscapingByref = true;
  }

  /// Retrieve the variable declaration from which this variable could
  /// be instantiated, if it is an instantiation (rather than a non-template).
  VarDecl *getTemplateInstantiationPattern() const;

  /// If this variable is an instantiated static data member of a
  /// class template specialization, returns the templated static data member
  /// from which it was instantiated.
  VarDecl *getInstantiatedFromStaticDataMember() const;

  /// If this variable is an instantiation of a variable template or a
  /// static data member of a class template, determine what kind of
  /// template specialization or instantiation this is.
  TemplateSpecializationKind getTemplateSpecializationKind() const;

  /// Get the template specialization kind of this variable for the purposes of
  /// template instantiation. This differs from getTemplateSpecializationKind()
  /// for an instantiation of a class-scope explicit specialization.
  TemplateSpecializationKind
  getTemplateSpecializationKindForInstantiation() const;

  /// If this variable is an instantiation of a variable template or a
  /// static data member of a class template, determine its point of
  /// instantiation.
  SourceLocation getPointOfInstantiation() const;

  /// If this variable is an instantiation of a static data member of a
  /// class template specialization, retrieves the member specialization
  /// information.
  MemberSpecializationInfo *getMemberSpecializationInfo() const;

  /// For a static data member that was instantiated from a static
  /// data member of a class template, set the template specialiation kind.
  void setTemplateSpecializationKind(TemplateSpecializationKind TSK,
                        SourceLocation PointOfInstantiation = SourceLocation());

  /// Specify that this variable is an instantiation of the
  /// static data member VD.
  void setInstantiationOfStaticDataMember(VarDecl *VD,
                                          TemplateSpecializationKind TSK);

  /// Retrieves the variable template that is described by this
  /// variable declaration.
  ///
  /// Every variable template is represented as a VarTemplateDecl and a
  /// VarDecl. The former contains template properties (such as
  /// the template parameter lists) while the latter contains the
  /// actual description of the template's
  /// contents. VarTemplateDecl::getTemplatedDecl() retrieves the
  /// VarDecl that from a VarTemplateDecl, while
  /// getDescribedVarTemplate() retrieves the VarTemplateDecl from
  /// a VarDecl.
  VarTemplateDecl *getDescribedVarTemplate() const;

  void setDescribedVarTemplate(VarTemplateDecl *Template);

  // Is this variable known to have a definition somewhere in the complete
  // program? This may be true even if the declaration has internal linkage and
  // has no definition within this source file.
  bool isKnownToBeDefined() const;

  /// Is destruction of this variable entirely suppressed? If so, the variable
  /// need not have a usable destructor at all.
  bool isNoDestroy(const ASTContext &) const;

  /// Would the destruction of this variable have any effect, and if so, what
  /// kind?
  QualType::DestructionKind needsDestruction(const ASTContext &Ctx) const;

  // Implement isa/cast/dyncast/etc.
  static bool classof(const Decl *D) { return classofKind(D->getKind()); }
  static bool classofKind(Kind K) { return K >= firstVar && K <= lastVar; }
};