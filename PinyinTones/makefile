#----- set the project name --------------------------------------------------------------
Proj = PinyinTones

#----- Include the PSDK's WIN32.MAK to pick up defines -----------------------------------
!include <win32.mak>
BINDIR=..\bin\$(OUTDIR)

#----- OUTDIR is defined in WIN32.MAK This is the name of the destination directory ------
all: $(BINDIR)\$(Proj).dll

LINK32_OBJS= \
	$(OUTDIR)\Compartment.obj \
	$(OUTDIR)\Composition.obj \
	$(OUTDIR)\DisplayAttribute.obj \
	$(OUTDIR)\DisplayAttributeInfo.obj \
	$(OUTDIR)\DisplayAttributeProvider.obj \
	$(OUTDIR)\Dllmain.obj \
	$(OUTDIR)\EndComposition.obj \
	$(OUTDIR)\EnumDisplayAttributeInfo.obj \
	$(OUTDIR)\Globals.obj \
	$(OUTDIR)\KeyEventSink.obj \
	$(OUTDIR)\KeyHandler.obj \
	$(OUTDIR)\LanguageBar.obj \
	$(OUTDIR)\PreComp.obj \
	$(OUTDIR)\Register.obj \
	$(OUTDIR)\Server.obj \
	$(OUTDIR)\StartComposition.obj \
	$(OUTDIR)\TextEditSink.obj \
	$(OUTDIR)\TextService.obj \
	$(OUTDIR)\ThreadMgrEventSink.obj

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

$(BINDIR) :
    if not exist "$(BINDIR)/$(NULL)" mkdir $(BINDIR)
	
#----- add the preprocessor definitions needed by this project ---------------------------

#----- set the libraries needed by this project ------------------------------------------
LINK_LIBS = $(dlllibs) $(guilibs) $(olelibs)

#--------------------- DLL ---------------------------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Linker options WIN32.MAK provides

# Build rule for DLL
$(BINDIR)\$(Proj).dll: $(OUTDIR) $(BINDIR) $(LINK32_OBJS) $(OUTDIR)\$(Proj).res
    $(link) $(ldebug) $(dllllflags) \
    $(LINK32_OBJS) $(OUTDIR)\$(Proj).res $(LINK_LIBS) \
    -base:0x20000000   \
    -out:$(BINDIR)\$(Proj).dll \
    -def:$(Proj).def

#--------------------- Compiling C/CPP Files ---------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Compiler options WIN32.MAK provides

{}.cpp{$(OUTDIR)}.obj:
    $(cc) $(cdebug) $(cflags) $(cvarsdll) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

#--------------------- Compiling Resource Files ------------------------------------------
#PLEASE REFER TO WIN32.MAK for the different Resource Compiler options WIN32.MAK provides

# Build rule for resource file
FILE=$(Proj)

$(OUTDIR)\$(FILE).res: .\$(FILE).rc $(OUTDIR)
    $(rc) $(rcflags) $(rcvars) /fo $(OUTDIR)\$(FILE).res .\$(FILE).rc

#--------------------- Clean Rule --------------------------------------------------------
# Rules for cleaning out those old files
clean:
        $(CLEANUP)
