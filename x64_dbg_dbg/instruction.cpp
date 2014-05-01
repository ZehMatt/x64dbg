#include "instruction.h"
#include "argument.h"
#include "variable.h"
#include "console.h"
#include "value.h"
#include "command.h"
#include "addrinfo.h"
#include "assemble.h"
#include "debugger.h"
#include "memory.h"
#include "x64_dbg.h"
#include "disasm_fast.h"
#include "reference.h"
#include "disasm_helper.h"

static bool bRefinit=false;

CMDRESULT cbBadCmd(int argc, char* argv[])
{
    uint value=0;
    int valsize=0;
    bool isvar=false;
    bool hexonly=false;
    if(valfromstring(*argv, &value, false, false, &valsize, &isvar, &hexonly)) //dump variable/value/register/etc
    {
        //dprintf("[DEBUG] valsize: %d\n", valsize);
        if(valsize)
            valsize*=2;
        else
            valsize=1;
        char format_str[deflen]="";
        if(isvar)// and *cmd!='.' and *cmd!='x') //prevent stupid 0=0 stuff
        {
            if(value>15 and !hexonly)
            {
                if(!valuesignedcalc()) //signed numbers
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"d)\n", valsize);
                dprintf(format_str, *argv, value, value);
            }
            else
            {
                sprintf(format_str, "%%s=%%.%d"fext"X\n", valsize);
                dprintf(format_str, *argv, value);
            }
        }
        else
        {
            if(value>15 and !hexonly)
            {
                if(!valuesignedcalc()) //signed numbers
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"d)\n", valsize);
                sprintf(format_str, "%%.%d"fext"X (%%"fext"ud)\n", valsize);
                dprintf(format_str, value, value);
            }
            else
            {
                sprintf(format_str, "%%.%d"fext"X\n", valsize);
                dprintf(format_str, value);
            }
        }
    }
    else //unknown command
    {
        dprintf("unknown command/expression: \"%s\"\n", *argv);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVar(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char arg2[deflen]="";
    argget(*argv, arg2, 1, true); //var value (optional)
    uint value=0;
    int add=0;
    if(*argv[1]=='$')
        add++;
    if(valfromstring(argv[1]+add, &value))
    {
        dprintf("invalid variable name \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!valfromstring(arg2, &value))
    {
        dprintf("invalid value \"%s\"\n", arg2);
        return STATUS_ERROR;
    }
    if(!varnew(argv[1], value, VAR_USER))
    {
        dprintf("error creating variable \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    else
    {
        if(value>15)
            dprintf("%s=%"fext"X (%"fext"ud)\n", argv[1], value, value);
        else
            dprintf("%s=%"fext"X\n", argv[1], value);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarDel(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    if(!vardel(argv[1], false))
        dprintf("could not delete variable \"%s\"\n", argv[1]);
    else
        dprintf("deleted variable \"%s\"\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMov(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments");
        return STATUS_ERROR;
    }
    uint set_value=0;
    if(!valfromstring(argv[2], &set_value))
    {
        dprintf("invalid src \"%s\"\n", argv[2]);
        return STATUS_ERROR;
    }
    bool isvar=false;
    uint temp=0;
    valfromstring(argv[1], &temp, true, false, 0, &isvar, 0);
    if(!isvar)
        isvar=vargettype(argv[1], 0);
    if(!isvar or !valtostring(argv[1], &set_value, true))
    {
        uint value;
        if(valfromstring(argv[1], &value)) //if the var is a value already it's an invalid destination
        {
            dprintf("invalid dest \"%s\"\n", argv[1]);
            return STATUS_ERROR;
        }
        varnew(argv[1], set_value, VAR_USER);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarList(int argc, char* argv[])
{
    char arg1[deflen]="";
    argget(*argv, arg1, 0, true);
    int filter=0;
    if(!_stricmp(arg1, "USER"))
        filter=VAR_USER;
    else if(!_stricmp(arg1, "READONLY"))
        filter=VAR_READONLY;
    else if(!_stricmp(arg1, "SYSTEM"))
        filter=VAR_SYSTEM;
    VAR* cur=vargetptr();
    if(!cur or !cur->name)
    {
        dputs("no variables");
        return STATUS_CONTINUE;
    }

    bool bNext=true;
    while(bNext)
    {
        char name[deflen]="";
        strcpy(name, cur->name);
        int len=strlen(name);
        for(int i=0; i<len; i++)
            if(name[i]==1)
                name[i]='/';
        uint value=(uint)cur->value.u.value;
        if(cur->type!=VAR_HIDDEN)
        {
            if(filter)
            {
                if(cur->type==filter)
                {
                    if(value>15)
                        dprintf("%s=%"fext"X (%"fext"ud)\n", name, value, value);
                    else
                        dprintf("%s=%"fext"X\n", name, value);
                }
            }
            else
            {
                if(value>15)
                    dprintf("%s=%"fext"X (%"fext"ud)\n", name, value, value);
                else
                    dprintf("%s=%"fext"X\n", name, value);
            }
        }
        cur=cur->next;
        if(!cur)
            bNext=false;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrChd(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    if(!DirExists(argv[1]))
    {
        dputs("directory doesn't exist");
        return STATUS_ERROR;
    }
    SetCurrentDirectoryA(argv[1]);
    dputs("current directory changed!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmt(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!commentset(addr, argv[2]))
    {
        dputs("error setting comment");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmtdel(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!commentdel(addr))
    {
        dputs("error deleting comment");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLbl(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!labelset(addr, argv[2]))
    {
        dputs("error setting label");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLbldel(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!labeldel(addr))
    {
        dputs("error deleting label");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkSet(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bookmarkset(addr))
    {
        dputs("failed to set bookmark!");
        return STATUS_ERROR;
    }
    dputs("bookmark set!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkDel(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bookmarkdel(addr))
    {
        dputs("failed to delete bookmark!");
        return STATUS_ERROR;
    }
    dputs("bookmark deleted!");
    return STATUS_CONTINUE;
}

CMDRESULT cbLoaddb(int argc, char* argv[])
{
    if(!dbload())
    {
        dputs("failed to load database from disk!");
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbSavedb(int argc, char* argv[])
{
    if(!dbsave())
    {
        dputs("failed to save database to disk!");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbAssemble(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid expression: \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!DbgMemIsValidReadPtr(addr))
    {
        dprintf("invalid address: "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    if(!assembleat(addr, argv[2]))
    {
        dprintf("failed to assemble \"%s\"\n", argv[2]);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbFunctionAdd(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint start=0;
    uint end=0;
    if(!valfromstring(argv[1], &start, false) or !valfromstring(argv[2], &end, false))
        return STATUS_ERROR;
    if(!functionadd(start, end, true))
    {
        dputs("failed to add function");
        return STATUS_ERROR;
    }
    dputs("function added!");
    return STATUS_CONTINUE;
}

CMDRESULT cbFunctionDel(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!functiondel(addr))
    {
        dputs("failed to delete function");
        return STATUS_ERROR;
    }
    dputs("function deleted!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmp(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint arg1=0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    uint arg2=0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    uint ezflag;
    uint bsflag;
    if(arg1==arg2)
        ezflag=1;
    else
        ezflag=0;
    if(valuesignedcalc()) //signed comparision
    {
        if((sint)arg1<(sint)arg2)
            bsflag=0;
        else
            bsflag=1;
    }
    else //unsigned comparision
    {
        if(arg1>arg2)
            bsflag=1;
        else
            bsflag=0;
    }
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    //dprintf("$_EZ_FLAG=%d, $_BS_FLAG=%d\n", ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrGpa(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    if(argc>=3)
        sprintf(newcmd, "%s:%s", argv[2], argv[1]);
    else
        sprintf(newcmd, "%s", argv[1]);
    uint result=0;
    if(!valfromstring(newcmd, &result, false))
        return STATUS_ERROR;
    varset("$RESULT", result, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAdd(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s+%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrAnd(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s&%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrDec(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s-1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrDiv(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s/%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrInc(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s+1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrMul(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s*%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrNeg(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s*-1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrNot(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,~%s", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrOr(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s|%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrRol(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s<%s", argv[1], argv[1], argv[2]);
    bool signedcalc=valuesignedcalc();
    valuesetsignedcalc(true); //rol = signed
    CMDRESULT res=cmddirectexec(dbggetcommandlist(), newcmd);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrRor(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s>%s", argv[1], argv[1], argv[2]);
    bool signedcalc=valuesignedcalc();
    valuesetsignedcalc(true); //ror = signed
    CMDRESULT res=cmddirectexec(dbggetcommandlist(), newcmd);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrShl(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s<%s", argv[1], argv[1], argv[2]);
    bool signedcalc=valuesignedcalc();
    valuesetsignedcalc(false); //shl = unsigned
    CMDRESULT res=cmddirectexec(dbggetcommandlist(), newcmd);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrShr(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s>%s", argv[1], argv[1], argv[2]);
    bool signedcalc=valuesignedcalc();
    valuesetsignedcalc(false); //shr = unsigned
    CMDRESULT res=cmddirectexec(dbggetcommandlist(), newcmd);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrSub(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s-%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrTest(int argc, char* argv[])
{
    //TODO: test
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint arg1=0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    uint arg2=0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    uint ezflag;
    uint bsflag=0;
    if(!(arg1&arg2))
        ezflag=1;
    else
        ezflag=0;
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    //dprintf("$_EZ_FLAG=%d, $_BS_FLAG=%d\n", ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrXor(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen]="";
    sprintf(newcmd, "mov %s,%s^%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrRefinit(int argc, char* argv[])
{
    GuiReferenceDeleteAllColumns();
    GuiReferenceAddColumn(sizeof(uint)*2, "Address");
    GuiReferenceAddColumn(0, "Data");
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    bRefinit=true;
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrRefadd(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bRefinit)
        cbInstrRefinit(argc, argv);
    int index=GuiReferenceGetRowCount();
    GuiReferenceSetRowCount(index+1);
    char addr_text[deflen]="";
    sprintf(addr_text, fhex, addr);
    GuiReferenceSetCellContent(index, 0, addr_text);
    GuiReferenceSetCellContent(index, 1, argv[2]);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

//reffind value[,page]
static bool cbRefFind(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!refinfo) //initialize
    {
        GuiReferenceDeleteAllColumns();
        GuiReferenceAddColumn(2*sizeof(uint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceReloadData();
        return true;
    }
    bool found=false;
    uint value=(uint)refinfo->userinfo;
    if((basicinfo->type&TYPE_VALUE)==TYPE_VALUE)
    {
        if(basicinfo->value.value==value)
            found=true;
    }
    if((basicinfo->type&TYPE_MEMORY)==TYPE_MEMORY)
    {
        if(basicinfo->memory.value==value)
            found=true;
    }
    if((basicinfo->type&TYPE_ADDR)==TYPE_ADDR)
    {
        if(basicinfo->addr==value)
            found=true;
    }
    if(found)
    {
        char addrText[20]="";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount+1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[2048]="";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
    }
    return found;
}

CMDRESULT cbInstrRefFind(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint value=0;
    if(!valfromstring(argv[1], &value, false))
        return STATUS_ERROR;
    uint addr=0;
    if(argc<3 or !valfromstring(argv[2], &addr))
        addr=GetContextData(UE_CIP);
    uint size;
    if(argc>=4)
    {
        if(!valfromstring(argv[3], &size))
            size=0;
    }
    uint ticks=GetTickCount();
    int found=reffind(addr, size, cbRefFind, (void*)value, false);
    dprintf("%u references in %ums\n", found, GetTickCount()-ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

//refstr [page]
bool cbRefStr(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!refinfo) //initialize
    {
        GuiReferenceDeleteAllColumns();
        GuiReferenceAddColumn(2*sizeof(uint), "Address");
        GuiReferenceAddColumn(64, "Disassembly");
        GuiReferenceAddColumn(0, "String");
        GuiReferenceSetSearchStartCol(2); //only search the strings
        GuiReferenceReloadData();
        return true;
    }
    bool found=false;
    STRING_TYPE strtype;
    char string[512]="";
    if(basicinfo->branch) //branches have no strings (jmp dword [401000])
        return false;
    if((basicinfo->type&TYPE_VALUE)==TYPE_VALUE)
    {
        if(disasmispossiblestring(basicinfo->value.value) and disasmgetstringat(basicinfo->value.value, &strtype, string, string, 500))
            found=true;
    }
    if((basicinfo->type&TYPE_MEMORY)==TYPE_MEMORY)
    {
        if(!found and disasmispossiblestring(basicinfo->memory.value) and disasmgetstringat(basicinfo->memory.value, &strtype, string, string, 500))
            found=true;
    }
    if(found)
    {
        char addrText[20]="";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount+1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[2048]="";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
        char dispString[1024]="";
        if(strtype==str_ascii)
            sprintf(dispString, "\"%s\"", string);
        else
            sprintf(dispString, "L\"%s\"", string);
        GuiReferenceSetCellContent(refinfo->refcount, 2, dispString);
    }
    return found;
}

CMDRESULT cbInstrRefStr(int argc, char* argv[])
{
    uint addr;
    if(argc<2 or !valfromstring(argv[1], &addr, true))
        addr=GetContextData(UE_CIP);
    uint size;
    if(argc>=3)
    {
        if(!valfromstring(argv[2], &size, true))
            size=0;
    }
    uint ticks=GetTickCount();
    int found=reffind(addr, size, cbRefStr, 0, false);
    dprintf("%u references in %ums\n", found, GetTickCount()-ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSetstr(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    varnew(argv[1], 0, VAR_USER);
    if(!vargettype(argv[1], 0))
    {
        dprintf("no such variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!varset(argv[1], argv[2], false))
    {
        dprintf("failed to set variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    char cmd[deflen]="";
    sprintf(cmd, "getstr \"%s\"", argv[1]);
    cmddirectexec(dbggetcommandlist(), cmd);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrGetstr(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    VAR_TYPE vartype;
    if(!vargettype(argv[1], &vartype))
    {
        dprintf("no such variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(vartype!=VAR_STRING)
    {
        dprintf("variable \"%s\" is not a string!\n", argv[1]);
        return STATUS_ERROR;
    }
    int size;
    if(!varget(argv[1], (char*)0, &size, 0) or !size)
    {
        dprintf("failed to get variable size \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    char* string=(char*)emalloc(size+1, "cbInstrGetstr:string");
    memset(string, 0, size+1);
    if(!varget(argv[1], string, &size, 0))
    {
        dprintf("failed to get variable data \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    dprintf("%s=\"%s\"\n", argv[1], string);
    efree(string, "cbInstrGetstr:string");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFind(int argc, char* argv[])
{
    if(argc<3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    char pattern[deflen]="";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0]=='#')
        strcpy(pattern, argv[2]+1);
    else
        strcpy(pattern, argv[2]);
    int len=strlen(pattern);
    if(pattern[len-1]=='#')
        pattern[len-1]='\0';
    uint size=0;
    uint base=memfindbaseaddr(fdProcessInfo->hProcess, addr, &size);
    if(!base)
    {
        dprintf("invalid memory address "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    unsigned char* data=(unsigned char*)emalloc(size, "cbInstrFind:data");
    if(!memread(fdProcessInfo->hProcess, (const void*)base, data, size, 0))
    {
        efree(data, "cbInstrFind:data");
        dputs("failed to read memory!");
        return STATUS_ERROR;
    }
    uint start=addr-base;
    uint find_size=0;
    if(argc>=4)
    {
        if(!valfromstring(argv[3], &find_size))
            find_size=size-start;
        if(find_size>(size-start))
            find_size=size-start;
    }
    else
        find_size=size-start;
    uint foundoffset=memfindpattern(data+start, find_size, pattern);
    uint result=0;
    if(foundoffset!=-1)
        result=addr+foundoffset;
    varset("$result", result, false);
    DbgCmdExec("$result");
    return STATUS_CONTINUE;
}
