#!/usr/bin/env python
import os, sys, psutil
import subprocess
import inspect
import time
import tempfile
import difflib
import re

# 🧬 Settings
script_dir = sys.path[0]
projectname = os.path.basename(os.path.dirname(script_dir))
if os.name=='nt':
    exe = f"../build/bin/win-x64-Release/{projectname}.exe"
    #exe = f"{projectname}.exe"
else:
    exe = f"../build/bin/{projectname}"




#----------------------------------------------------------------------------
GRAY = '\033[90m';
RED = '\033[91m';
GREEN = '\033[92m';
YELLOW = '\033[93m';
BLUE = '\033[94m';
MAGENTA = '\033[95m';
CYAN = '\033[96m';
END = '\033[0m';

#----------------------------------------------------------------------------
def set_title(title):
    if os.name=='nt':
        os.system(f"title {title}")
    else:
        sys.stdout.write(f"\x1b]2;{title}\x07")

#----------------------------------------------------------------------------
def is_manual_mode():
    return len(sys.argv)>1 and sys.argv[1].lower().startswith('man');

#----------------------------------------------------------------------------
def pause():
    input(f'{YELLOW}Press <ENTER> to continue{END}')

#----------------------------------------------------------------------------
def closing():
    parent_process = psutil.Process(os.getpid()).parent().name()
    temp_parents = re.compile(r"(?i)^(explorer.*|.*terminal)$")
    if temp_parents.match(parent_process):
        print(f'{GRAY}Closing... ({parent_process}){END}')
        time.sleep(3)

#----------------------------------------------------------------------------
def wait_for_keypress():
    if os.name=='nt':
        import msvcrt
        key = chr(msvcrt.getch()[0])
    else:
        import termios
        fd = sys.stdin.fileno()
        oldterm = termios.tcgetattr(fd)
        newattr = termios.tcgetattr(fd)
        newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
        termios.tcsetattr(fd, termios.TCSANOW, newattr)
        try: key = sys.stdin.read(1)[0]
        except IOError: pass
        finally: termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
    return key

#----------------------------------------------------------------------------
def launch(command_and_args):
    print(f"{GRAY}")
    start_time_s = time.process_time()
    return_code = subprocess.call(command_and_args)
    exec_time_s = time.process_time() - start_time_s
    exec_time_ms = 1000.0 * exec_time_s
    print(f"{END}{command_and_args[0]} returned: {GREEN if return_code==0 else RED}{return_code}{END} after {CYAN}{exec_time_ms:.2f}{END}ms")
    return return_code, exec_time_ms

#----------------------------------------------------------------------------
def create_text_file(dir, fname, text_content, encoding):
    file_path = os.path.join(dir, fname)
    with open(file_path, 'wb') as f:
        f.write( text_content.encode(encoding) )
    return file_path

#----------------------------------------------------------------------------
class TextFile:
    def __init__(self, file_name=None, content=None, encoding='utf-8'):
        self.file_name = file_name
        self.content = content.replace('#file_name#', file_name)
        self.encoding = encoding
    def create_in(self, dir):
        return create_text_file(dir, self.file_name, self.content, self.encoding)
    def create_with_content_in(self, dir, provided_content):
        return create_text_file(dir, self.file_name, provided_content, self.encoding)

#----------------------------------------------------------------------------
class TemporaryTextFile:
    def __init__(self, txt_file, directory=None):
        if not isinstance(txt_file, TextFile):
            raise ValueError("Argument of TemporaryTextFile must be an instance of the 'TextFile' named tuple")
        self.directory = directory if directory else tempfile.gettempdir()
        self.path = create_text_file(self.directory, txt_file.file_name, txt_file.content, txt_file.encoding)
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        try: os.remove(self.path)
        except: pass

#----------------------------------------------------------------------------
class Directory:
    def __init__(self, name, root=""):
        self.path = os.path.join(root, name)
        os.mkdir(self.path)
    def decl_file(self, file_name):
        return os.path.join(self.path, file_name)

#----------------------------------------------------------------------------
def check_strings_equality(str1, str2):
    def show_char(ch):
        return ch if ch.isprintable() and not ch.isspace() else repr(ch)
    if str1 == str2:
        return True
    else:
        diff = difflib.ndiff(str1, str2)
        result = []
        for i, s in enumerate(diff):
            if s[0] == ' ':
                result.append(GRAY + s[-1] + END) # equal parts
            elif s[0] == '-': # missing characters
                result.append(MAGENTA + show_char(s[-1]) + END)
            elif s[0] == '+': # surplus characters
                result.append(RED + show_char(s[-1]) + END)
        print(''.join(result))
        return False


#----------------------------------------------------------------------------
def textfile_content_is(file_path, encoding, expected):
    with open(file_path, 'rb') as file:
        content = file.read().decode(encoding)
        return check_strings_equality(content,expected)
        # print(f'{GRAY}expected:{END}\n{expected}')
        # print(f'{GRAY}actual:{END}\n{content}')

#----------------------------------------------------------------------------
def check_file_content(path, file):
    if os.path.isfile(path):
        if textfile_content_is(path, file.encoding, file.content):
            return True
        else:
            print(f'{RED}{path} {MAGENTA}content doesn\'t match{END}')
            #if self.manual_mode :
                #show_text_file(path)
                #time.sleep(0.5)
    else:
        print(f'{RED}{path} {MAGENTA}not created{END}')

#----------------------------------------------------------------------------
def show_text_file(file_path):
    print(f"opening {file_path}")
    subprocess.run(['start' if os.name=='nt' else 'xdg-open', file_path])

#----------------------------------------------------------------------------
def show_text_files_comparison(left_file, right_file):
    if os.name=='nt':
        command_and_args = [os.path.expandvars("%ProgramFiles%/WinMerge/WinMergeU.exe"), "/e", "/u", "/wl", "/x", left_file, right_file]
    else:
        command_and_args = ["meld", left_file, right_file]
    #print(f"comparing {left_file} {right_file}")
    subprocess.call(command_and_args)

#----------------------------------------------------------------------------
def create_plclib_full_content(name, inner_content):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'
            '<plcLibrary schemaVersion="2.8">\n'
           f'    <lib version="0.5.0" name="{name}">{inner_content}</lib>\n'
            '</plcLibrary>\n')

#----------------------------------------------------------------------------
class Test:
    keys = iter("1234567890qwertyuiopasdfghjklzxcvbnm")
    def __init__(self, title, action):
        try:
            self.menu_key = next(self.keys)
        except StopIteration:
            print(f'{RED}No more keys available for test menu!{END}')
            return
        self.title = title
        self.action = action
    def run(self):
        return self.action()
#----------------------------------------------------------------------------
class Tests:
    def __init__(self, manual_mode):
        self.list = []
        self.manual_mode = manual_mode
        prefix_auto = 'test_'
        prefix_just_man = 'testman_'
        for name, func in inspect.getmembers(self, inspect.ismethod):
            if name.startswith(prefix_auto):
                self.list.append(Test(name[len(prefix_auto):], func))
            elif manual_mode and name.startswith(prefix_just_man) :
                self.list.append(Test(name[len(prefix_just_man):], func))


    #========================================================================
    def test_convert_dupes(self):
        pll = TextFile( 'dupes.pll',
                        'FUNCTION_BLOCK fb\n'
                        '\n'
                        '{ DE:"a function block" }\n'
                        '\n'
                        '   VAR_EXTERNAL\n'
                        '   var1 : BOOL; { DE:"variable 1" }\n'
                        '   var1 : BOOL; { DE:"variable 1" }\n'
                        '   var2 : BOOL; { DE:"variable 2" }\n'
                        '   END_VAR\n'
                        '\n'
                        '   { CODE:ST }\n'
                        '    var2 := var1;\n'
                        '\n'
                        'END_FUNCTION_BLOCK\n' )
        with TemporaryTextFile(pll) as pll_file:
            ret_code, exec_time_ms = launch([exe, "convert", "-F", "-v" if self.manual_mode else "-q", pll_file.path])
            time.sleep(0.5) # Give the time to show the bad file
            return ret_code==2 # should complain about duplicate variable


    #========================================================================
    def test_convert_empty_pll(self):
        pll = TextFile('empty.pll', '\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            pll_path = pll.create_in(temp_dir)
            ret_code, exec_time_ms = launch([exe, "convert", "-F", "-v" if self.manual_mode else "-q", pll_path])
            return ret_code==1 # empty pll should raise an issue


    #========================================================================
    def test_convert_h(self):
        h = TextFile('file.h',
                    '\n'
                    '#define Pos vq32 // Position\n'
                    '#define aaa bbb // not exported\n'
                    '#define PI 3.14 // [LREAL] C/d\n'
                    '#define G 42 // a local constant\n'
                    '#define Density 2500 // [DINT] exported\n'
                    '\n' )
        pll = TextFile('file.pll',
                    '(*\n'
                    '\tname: file\n'
                    '\tdescr: PLC library\n'
                    '\tversion: 1.0.0\n'
                    '\tauthor: pll::write()\n'
                    '\tglobal-variables: 1\n'
                    '\tglobal-constants: 2\n'
                    '*)\n'
                    '\n'
                    '\n'
                    '\n'
                    '\t(****************************)\n'
                    '\t(*                          *)\n'
                    '\t(*     GLOBAL VARIABLES     *)\n'
                    '\t(*                          *)\n'
                    '\t(****************************)\n'
                    '\n'
                    '\tVAR_GLOBAL\n'
                    '\t{G:"Header_Variables"}\n'
                    '\tPos AT %MD500.32 : DINT; { DE:"Position" }\n'
                    '\tEND_VAR\n'
                    '\n'
                    '\n'
                    '\n'
                    '\t(****************************)\n'
                    '\t(*                          *)\n'
                    '\t(*     GLOBAL CONSTANTS     *)\n'
                    '\t(*                          *)\n'
                    '\t(****************************)\n'
                    '\n'
                    '\tVAR_GLOBAL CONSTANT\n'
                    '\t{G:"Header_Constants"}\n'
                    '\tPI : LREAL := 3.14; { DE:"C/d" }\n'
                    '\tDensity : DINT := 2500; { DE:"exported" }\n'
                    '\tEND_VAR\n')
        plclib = TextFile('file.plclib',
                    '<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'
                    '<plcLibrary schemaVersion="2.8">\n'
                    '\t<lib version="1.0.0" name="file" fullXml="true">\n'
                    '\t\t<!-- author="plclib::write()" -->\n'
                    '\t\t<descr>PLC library</descr>\n'
                    '\t\t<!--\n'
                    '\t\t\tglobal-variables: 1\n'
                    '\t\t\tglobal-constants: 2\n'
                    '\t\t-->\n'
                    '\t\t<libWorkspace>\n'
                    '\t\t\t<folder name="file" id="1040">\n'
                    '\t\t\t\t<GlobalVars name="Header_Constants"/>\n'
                    '\t\t\t\t<GlobalVars name="Header_Variables"/>\n'
                    '\t\t\t</folder>\n'
                    '\t\t</libWorkspace>\n'
                    '\t\t<globalVars>\n'
                    '\t\t\t<group name="Header_Variables" excludeFromBuild="FALSE" excludeFromBuildIfNotDef="" version="1.0.0">\n'
                    '\t\t\t\t<var name="Pos" type="DINT">\n'
                    '\t\t\t\t\t<descr>Position</descr>\n'
                    '\t\t\t\t\t<address type="M" typeVar="D" index="500" subIndex="32"/>\n'
                    '\t\t\t\t</var>\n'
                    '\t\t\t</group>\n'
                    '\t\t</globalVars>\n'
                    '\t\t<retainVars/>\n'
                    '\t\t<constantVars>\n'
                    '\t\t\t<group name="Header_Constants" excludeFromBuild="FALSE" excludeFromBuildIfNotDef="" version="1.0.0">\n'
                    '\t\t\t\t<const name="PI" type="LREAL">\n'
                    '\t\t\t\t\t<descr>C/d</descr>\n'
                    '\t\t\t\t\t<initValue>3.14</initValue>\n'
                    '\t\t\t\t</const>\n'
                    '\t\t\t\t<const name="Density" type="DINT">\n'
                    '\t\t\t\t\t<descr>exported</descr>\n'
                    '\t\t\t\t\t<initValue>2500</initValue>\n'
                    '\t\t\t\t</const>\n'
                    '\t\t\t</group>\n'
                    '\t\t</constantVars>\n'
                    '\t\t<iecVarsDeclaration>\n'
                    '\t\t\t<group name="Header_Constants">\n'
                    '\t\t\t\t<iecDeclaration active="FALSE"/>\n'
                    '\t\t\t</group>\n'
                    '\t\t\t<group name="Header_Variables">\n'
                    '\t\t\t\t<iecDeclaration active="FALSE"/>\n'
                    '\t\t\t</group>\n'
                    '\t\t</iecVarsDeclaration>\n'
                    '\t\t<functions/>\n'
                    '\t\t<functionBlocks/>\n'
                    '\t\t<programs/>\n'
                    '\t\t<macros/>\n'
                    '\t\t<structs/>\n'
                    '\t\t<typedefs/>\n'
                    '\t\t<enums/>\n'
                    '\t\t<subranges/>\n'
                    '\t</lib>\n'
                    '</plcLibrary>\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            h_path = h.create_in(temp_dir)
            out_dir = Directory("out", temp_dir)
            ret_code, exec_time_ms = launch([exe, "convert", "-v" if self.manual_mode else "-q", h_path, "--to", out_dir.path])
            out_pll = out_dir.decl_file(pll.file_name);
            out_plclib = out_dir.decl_file(plclib.file_name);
            return ret_code==0 and exec_time_ms<16 and check_file_content(out_pll, pll) and check_file_content(out_plclib, plclib)


    #========================================================================
    def test_convert_name_clash(self):
        h = TextFile('file.h', '\n')
        pll = TextFile('file.pll', '\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            h_path = h.create_in(temp_dir)
            pll_path = pll.create_in(temp_dir)
            out_dir = Directory("out", temp_dir)
            ret_code, exec_time_ms = launch([exe, "convert", "-v" if self.manual_mode else "-q", h_path, pll_path, "--to", out_dir.path])
            return ret_code==2 # should complain about name clash


    #========================================================================
    def test_convert_no_file(self):
        ret_code, exec_time_ms = launch([exe, "convert", "-v" if self.manual_mode else "-q"])
        return ret_code==2 # no file given should give a fatal error


    #========================================================================
    def test_convert_overwrite(self):
        h = TextFile('file.h', '\n')
        plclib = TextFile('file.plclib', '\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            h_path = h.create_in(temp_dir)
            plclib.create_in(temp_dir)
            ret_code, exec_time_ms = launch([exe, "convert", "-v" if self.manual_mode else "-q", h_path, "--to", temp_dir])
            return ret_code==2 # should complain about overwrite existing


    #========================================================================
    def test_convert_same_dir(self):
        h1 = TextFile('file1.h', '\n')
        h2 = TextFile('file2.h', '\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            h1.create_in(temp_dir)
            h2.create_in(temp_dir)
            ret_code, exec_time_ms = launch([exe, "convert", "-F", "-v" if self.manual_mode else "-q", os.path.join(temp_dir,"*.h"), "--to", temp_dir])
            return ret_code==2 # should complain about outputting in the input directory


    #========================================================================
    def test_update_bad_plclib(self):
        bad_plclib = TextFile( 'bad.plclib',
                               '<lib>forbidden nested</lib>' )
        prj = TextFile( 'badlib.ppjs',
                        '<plcProject>\n'
                        '    <libraries>\n'
                       f'        <lib link="true" name="{bad_plclib.file_name}">\n'
                        '            <![CDATA[...]]>\n'
                        '        </lib>\n'
                        '    </libraries>\n'
                        '</plcProject>\n',
                        'utf-16' )
        with tempfile.TemporaryDirectory() as temp_dir:
            bad_plclib.create_with_content_in(temp_dir, create_plclib_full_content(bad_plclib.file_name, bad_plclib.content))
            prj_path = prj.create_in(temp_dir)
            if self.manual_mode:
                ret_code, exec_time_ms = launch([exe, "update", "-v", prj_path])
                time.sleep(0.5) # Give the time to show the bad file
            else:
                ret_code, exec_time_ms = launch([exe, "update", "-q", prj_path])
            return ret_code==2 # bad library should give a fatal error


    #========================================================================
    def test_update_bad_prj(self):
        prj = TextFile( 'bad.ppjs',
                        '<plcProject>\n'
                        '    <libraries>\n'
                        '        <lib link="true" name="unclosed">\n'
                        '            <![CDATA[...]]>\n'
                        '    </libraries>\n'
                        '</plcProject>\n',
                        'utf-16' )
        with TemporaryTextFile(prj) as prj_file:
            if self.manual_mode:
                ret_code, exec_time_ms = launch([exe, "update", "-v", prj_file.path])
                time.sleep(0.5) # Give the time to show the bad file
            else:
                ret_code, exec_time_ms = launch([exe, "update", "-q", prj_file.path])
            return ret_code==2 # bad project should give a fatal error


    #========================================================================
    def test_update_empty(self):
        prj = TextFile('empty.ppjs', '\n')
        with TemporaryTextFile(prj) as prj_file:
            ret_code, exec_time_ms = launch([exe, "update", "-v" if self.manual_mode else "-q", prj_file.path])
            return ret_code==2 # empty file should give a fatal error


    #========================================================================
    def test_update_no_file(self):
        ret_code, exec_time_ms = launch([exe, "update", "-v" if self.manual_mode else "-q"])
        return ret_code==2 # no file given should give a fatal error


    #========================================================================
    def test_update_no_libs(self):
        prj = TextFile( 'nolibs.ppjs',
                        '<plcProject>\n'
                        '    <libraries>\n'
                        '    </libraries>\n'
                        '</plcProject>\n' )
        with TemporaryTextFile(prj) as prj_file:
            ret_code, exec_time_ms = launch([exe, "update", "-v" if self.manual_mode else "-q", prj_file.path])
            return ret_code==1 # no libraries should raise an issue


    #========================================================================
    def test_update_nonexistent_lib(self):
        pll1 = TextFile( 'lib1.pll',
                         'content of pll1' )
        prj = TextFile( 'brokenlib.ppjs',
                        '<plcProject>\n'
                        '    <libraries>\n'
                        '        <lib link="true" name="not-existing.pll">\n'
                        '            <![CDATA[...]]>\n'
                        '        </lib>\n'
                       f'        <lib link="true" name="{pll1.file_name}">x</lib>\n'
                        '    </libraries>\n'
                        '</plcProject>\n',
                        'utf-16' )
        with tempfile.TemporaryDirectory() as temp_dir:
            pll1.create_in(temp_dir)
            prj_path = prj.create_in(temp_dir)
            ret_code, exec_time_ms = launch([exe, "update", "-v" if self.manual_mode else "-q", prj_path])
            expected = prj.content.replace(f'name="{pll1.file_name}">x', f'name="{pll1.file_name}"><![CDATA[{pll1.content}]]>')
            content_equal = textfile_content_is(prj_path, prj.encoding, expected)
            return ret_code==1 # nonexistent library should raise an issue


    #========================================================================
    def test_update_plcprj(self):
        defvar = TextFile( 'defvar.plclib', 'vbTrue' )
        iomap = TextFile( 'iomap.plclib', 'din81' )
        lib_subfolder = 'generated-libs';
        prj = TextFile( '~logiclab.plclib',
            '<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n'
            '<plcProject caseSensitive="true" debugMode="false">\n'
            '   <image>LogicLab.imgx</image>\n'
            '   <commSettings>SiaxProComm:0,1000,T#TCPIP:192.168.232.110/12004,5000</commSettings>\n'
            '   <sources>\n'
            '       <main>\n'
            '           <resources>\n'
            '               <resource name="SiaxM32" processor="ColdFire">\n'
            '                   <task name="Main" interval="t#0us" priority="0" type="cyclic" descr=""/>\n'
            '                   <task name="Boot" interval="t#0us" priority="1" type="cyclic" descr=""/>\n'
            '               </resource>\n'
            '           </resources>\n'
            '           <tasks>\n'
            '               <task name="Main">\n'
            '                   <program>IOimage</program>\n'
            '               </task>\n'
            '               <task name="Boot">\n'
            '                   <program>Init</program>\n'
            '               </task>\n'
            '           </tasks>\n'
            '           <globalVars/>\n'
            '           <retainVars/>\n'
            '           <constantVars/>\n'
            '           <functions/>\n'
            '           <functionBlocks/>\n'
            '           <programs>\n'
            '               <program name="Init" version="1.0.0" creationDate="1626179448" lastModifiedDate="1626254172" excludeFromBuild="FALSE" excludeFromBuildIfNotDef="">\n'
            '                   <vars/>\n'
            '                   <iecDeclaration active="FALSE"/>\n'
            '                   <sourceCode type="ST">\n'
            '                       <![CDATA[...]]>\n'
            '                   </sourceCode>\n'
            '               </program>\n'
            '           </programs>\n'
            '           <macros/>\n'
            '           <structs/>\n'
            '           <typedefs/>\n'
            '           <enums/>\n'
            '           <subranges/>\n'
            '           <interfaces/>\n'
            '       </main>\n'
            '       <target id="SiaxM32_1p0" name="LogicLab-plclib.tgt" fullXml="false">\n'
            '           <![CDATA[...]]>\n'
            '       </target>\n'
            '       <libraries>\n'
           f'           <lib version="0.5.1" name="{lib_subfolder}/{defvar.file_name}" fullXml="true" link="true">#DEFVAR#</lib>\n'
           f'           <lib name="{lib_subfolder}/{iomap.file_name}" fullXml="false" link="true">#IOMAP#</lib>\n'
            '       </libraries>\n'
            '       <aux/>\n'
            '       <workspace>\n'
            '           <root name="LogicLab-plclib" nextID="3682184">\n'
            '               <Pou name="Init" id="2"/>\n'
            '           </root>\n'
            '       </workspace>\n'
            '   </sources>\n'
            '   <codegen warningEnable="true">\n'
            '       <disabledWarnings>\n'
            '           <warning>G0015</warning>\n'
            '           <warning>G0065</warning>\n'
            '       </disabledWarnings>\n'
            '       <applicationDataBlocks/>\n'
            '   </codegen>\n'
            '   <simWorkspaces/>\n'
            '   <debug watchRefresh="20"/>\n'
            '   <editor useOldGridSize="false"/>\n'
            '   <userDef>\n'
            '       <release>LogicLab</release>\n'
            '       <author>MG</author>\n'
            '       <note>m32 project</note>\n'
            '       <version>1.0.0</version>\n'
            '       <password disabled="false"/>\n'
            '   </userDef>\n'
            '   <downloadSequence sourceCode="onPLCApplicationDownload" debugSymbols="onPLCApplicationDownload"/>\n'
            '   <commands>\n'
            '       <postbuild/>\n'
            '       <postdown/>\n'
            '       <predown/>\n'
            '   </commands>\n'
            '   <resources/>\n'
            '</plcProject>\n',
            'utf-8' )
        with tempfile.TemporaryDirectory() as temp_dir:
            libs_dir = os.path.join(temp_dir, lib_subfolder)
            os.mkdir(libs_dir)
            defvar.create_with_content_in(libs_dir, create_plclib_full_content(defvar.file_name, defvar.content))
            iomap.create_with_content_in(libs_dir, create_plclib_full_content(iomap.file_name, iomap.content))
            prj_path = prj.create_in(temp_dir)
            out_path = os.path.join(temp_dir, "updated.xml")
            ret_code, exec_time_ms = launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", out_path, prj_path])
            expected = ( prj.content.replace('#DEFVAR#', defvar.content)
                                    .replace('#IOMAP#', iomap.content) )
            content_equal = textfile_content_is(out_path, prj.encoding, expected)
            if self.manual_mode and os.path.isfile(out_path):
                show_text_files_comparison(prj_path, out_path)
            return ret_code==0 and content_equal and exec_time_ms<16


    #========================================================================
    def test_update_ppjs(self):
        defvar = TextFile( 'defvar.pll', 'vbTrue' )
        iomap = TextFile( 'iomap.pll', 'din81' )
        lib_subfolder = 'generated-libs';
        prj = TextFile( '~logiclab.ppjs',
            '<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n'
            '<plcProject caseSensitive="true" debugMode="false">\n'
            '    <image>LogicLab.imgx</image>\n'
            '    <commSettings>SiaxProComm:0,1000,T#TCPIP:192.168.232.110/12003,5000</commSettings>\n'
            '    <sources>\n'
            '        <main>\n'
            '            <![CDATA[...]]>\n'
            '        </main>\n'
            '        <target id="SiaxM32_1p0" name="LogicLab.tgt">\n'
            '            <![CDATA[...]]>\n'
            '        </target>\n'
            '        <libraries>\n'
           f'            <lib link="true" name="{lib_subfolder}/{defvar.file_name}"><![CDATA[#DEFVAR#]]></lib>\n'
           f'            <lib link="true" name="{lib_subfolder}/{iomap.file_name}"><![CDATA[#IOMAP#]]></lib>\n'
            '        </libraries>\n'
            '        <aux/>\n'
            '    </sources>\n'
            '    <codegen warningEnable="true">\n'
            '        <disabledWarnings/>\n'
            '    </codegen>\n'
            '    <simWorkspaces/>\n'
            '    <debug watchRefresh="20"/>\n'
            '    <editor useOldGridSize="false"/>\n'
            '    <userDef>\n'
            '        <release>LogicLab</release>\n'
            '        <author>MG</author>\n'
            '        <note>m32 project</note>\n'
            '        <version>0.1</version>\n'
            '        <password disabled="false"></password>\n'
            '    </userDef>\n'
            '    <downloadSequence sourceCode="onPLCApplicationDownload" debugSymbols="onPLCApplicationDownload"/>\n'
            '    <commands>\n'
            '        <postbuild>"%APPLPATH%\\CmpAlPlc" %PRJBASENAME% %PRJPATH%</postbuild>\n'
            '        <postdown></postdown>\n'
            '    </commands>\n'
            '    <resources/>\n'
            '</plcProject>\n',
            'utf-16' )
        with tempfile.TemporaryDirectory() as temp_dir:
            libs_dir = os.path.join(temp_dir, lib_subfolder)
            os.mkdir(libs_dir)
            defvar.create_in(libs_dir)
            iomap.create_in(libs_dir)
            prj_path = prj.create_in(temp_dir)
            out_path = os.path.join(temp_dir, "updated.xml")
            ret_code, exec_time_ms = launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", out_path, prj_path])
            expected = ( prj.content.replace('#DEFVAR#', defvar.content)
                                    .replace('#IOMAP#', iomap.content) )
            content_equal = textfile_content_is(out_path, prj.encoding, expected)
            if self.manual_mode and os.path.isfile(out_path):
                show_text_files_comparison(prj_path, out_path)
            return ret_code==0 and content_equal and exec_time_ms<16


    #========================================================================
    def test_update_same_prj(self):
        prj = TextFile( 'same.ppjs',
                        '<plcProject>\n'
                        '    <libraries>\n'
                        '    </libraries>\n'
                        '</plcProject>\n',
                        'utf-16' )
        with TemporaryTextFile(prj) as prj_file:
            ret_code, exec_time_ms = launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", prj_file.path, prj_file.path])
            return ret_code==2 # same output file should give a fatal error


    #========================================================================
    def test_update_simple_prj(self):
        pll1 = TextFile( 'lib1.pll',
                         'content of pll1' )
        pll2 = TextFile( 'lib2.pll',
                         'content of pll2',
                         'utf-8-sig' )
        plclib1 = TextFile( 'lib1.plclib',
                            'content of plclib1' )
        plclib2 = TextFile( 'lib2.plclib',
                            'content of plclib2',
                            'utf-8-sig' )
        prj = TextFile( 'original.xml',
                        '<plcProject>\n'
                        '    <libraries>\n'
                       f'        <lib link="true" name="{pll1.file_name}"><![CDATA[prev content]]></lib>\n'
                       f'        <lib link="true" name="{pll2.file_name}"></lib>\n'
                       f'        <lib link="true" name="{plclib1.file_name}">prev content</lib>\n'
                       f'        <lib link="true" name="{plclib2.file_name}"></lib>\n'
                        '    </libraries>\n'
                        '</plcProject>\n',
                        'utf-16' )
        with tempfile.TemporaryDirectory() as temp_dir:
            pll1.create_in(temp_dir)
            pll2.create_in(temp_dir)
            plclib1.create_with_content_in(temp_dir, create_plclib_full_content(plclib1.file_name, plclib1.content))
            plclib2.create_with_content_in(temp_dir, create_plclib_full_content(plclib2.file_name, plclib2.content))
            prj_path = prj.create_in(temp_dir)
            out_path = os.path.join(temp_dir, "updated.xml")
            ret_code, exec_time_ms = launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", out_path, prj_path])
            expected = ( prj.content.replace(f'name="{pll1.file_name}"><![CDATA[prev content]]>', f'name="{pll1.file_name}"><![CDATA[{pll1.content}]]>')
                                    .replace(f'name="{pll2.file_name}">', f'name="{pll2.file_name}"><![CDATA[{pll2.content}]]>')
                                    .replace(f'name="{plclib1.file_name}">prev content', f'name="{plclib1.file_name}">{plclib1.content}')
                                    .replace(f'name="{plclib2.file_name}">', f'name="{plclib2.file_name}">{plclib2.content}') )
            content_equal = textfile_content_is(out_path, prj.encoding, expected)
            if self.manual_mode and os.path.isfile(out_path):
                show_text_files_comparison(prj_path, out_path)
            return ret_code==0 and content_equal and exec_time_ms<16


    #========================================================================
    def testman_update_strato_ppjs(self):
        prj = os.path.expandvars("%UserProfile%/Macotec/Machines/m32-Strato/sde/PLC/LogicLab/LogicLab.ppjs")
        out = "~updated.xml"
        with tempfile.TemporaryDirectory() as temp_dir:
            out = os.path.join(temp_dir, out)
            launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", out, prj])
            if self.manual_mode and os.path.isfile(out):
                show_text_files_comparison(prj, out)
            return True


    #========================================================================
    def testman_update_strato_plcprj(self):
        prj = os.path.expandvars("%UserProfile%/Macotec/Machines/m32-Strato/sde/PLC/LogicLab/LogicLab-plclib.plcprj")
        out = "~updated.xml"
        with tempfile.TemporaryDirectory() as temp_dir:
            out = os.path.join(temp_dir, out)
            launch([exe, "update", "-vF" if self.manual_mode else "-qF", "-o", out, prj])
            if self.manual_mode and os.path.isfile(out):
                show_text_files_comparison(prj, out)
            return True



#----------------------------------------------------------------------------
#def create_tests_array(manual_mode):
#    tests = [
#        Test("Convert_no_file", test_convert_no_file),
#        Test("Update_no_file", test_update_no_file),
#        Test("Convert_empty_pll", test_convert_empty_pll),
#        Test("Update_empty", test_update_empty),
#        Test("Update_no_libs", test_update_no_libs),
#        Test("Convert_same_dir", test_convert_same_dir),
#        Test("Convert_name_clash", test_convert_name_clash),
#        Test("Update_same_prj", test_update_same_prj),
#        Test("Update_bad_prj", test_update_bad_prj),
#        Test("Update_bad_lib", test_update_bad_plclib),
#        Test("Update_nonexistent_lib", test_update_nonexistent_lib),
#        Test("Update_simple", test_update_simple_prj),
#        Test("Update_ppjs", test_update_ppjs),
#        Test("Update_plcprj", test_update_plcprj)
#       ]
#    if manual_mode:
#        tests += [
#            Test("Update_strato_ppjs", test_update_strato_ppjs),
#            Test("Update_strato_plcprj", test_update_strato_plcprj)
#           ]
#
#    return tests





#----------------------------------------------------------------------------
def run_tests(tests_array):
    tests_results = []
    fails = 0
    for test in tests_array:
        print(f'\n-------------- {GRAY}test {YELLOW}{test.title}{END}')
        if test.run() :
            tests_results.append(f'{GREEN}[passed] {test.title}{END}')
        else:
            tests_results.append(f'{RED}[failed] {test.title}{END}')
            fails += 1
    # Print results
    print('\n-------------------------------------')
    for result_entry in tests_results:
        print(result_entry)
    return fails

#----------------------------------------------------------------------------
def ask_what_to_do(tests_array):
    def print_menu(tests_array):
        print("\n____________________________________")
        for test in tests_array:
            print(f"{YELLOW}[{test.menu_key}]{END}{test.title}", end=' ')
        print(f"{YELLOW}[ESC]{END}Quit")
    def print_selected_entry(key,name):
        print(f'{CYAN}{repr(key)} {BLUE}{name}{END}')
    def get_test(key_char, tests_array):
        return [test for test in tests_array if test.menu_key==key_char]

    print_menu(tests_array)
    key_char = wait_for_keypress()
    if selected:=get_test(key_char, tests_array):
        for test in selected:
            print_selected_entry(key_char,test.title)
            if test.run() :
                print(f'{GREEN}[passed] {test.title}{END}')
            else:
                print(f'{RED}[failed] {test.title}{END}')
    else:
        match key_char:
            case '\x1b' | '\x03':
                print_selected_entry(key_char,'Quit')
                return False
            case _:
                print(f"{RED}{repr(key_char)} is not a valid option{END}")
    return True


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)
    if not os.path.isfile(exe):
        print(f"{RED}{exe} not found!{END}")
        pause()
        sys.exit(1)
    tests = Tests(is_manual_mode());
    if tests.manual_mode:
        print(f'{exe} (manual)')
        while ask_what_to_do(tests.list): pass
    else:
        print(f'{exe} (auto)')
        fails = run_tests(tests.list)
        if fails>0:
            print(f"\n{RED}Had {fails} fails!{END}")
            pause()
        else:
            print(f"\n{GREEN}All {len(tests.list)} tests passed{END}")
            closing()
        return fails

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
