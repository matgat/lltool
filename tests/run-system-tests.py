#!/usr/bin/env python
import os, sys
from collections import namedtuple
import subprocess
import time
import tempfile

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
    return len(sys.argv)>1 and  sys.argv[1].lower().startswith('man');

#----------------------------------------------------------------------------
def launch(command_and_args):
    print(f"{GRAY}")
    return_code = subprocess.call(command_and_args)
    print(f"{END}{command_and_args[0]} returned: {YELLOW}{return_code}{END}")
    return return_code

#----------------------------------------------------------------------------
def pause():
    input(f'{YELLOW}Press <ENTER> to continue{END}')

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
def create_text_file(file_path, text_content, encoding):
    with open(file_path, 'wb') as f:
        f.write(text_content.encode(encoding))

#----------------------------------------------------------------------------
TextFile = namedtuple('TextFile', ['file_name', 'content', 'encoding'], defaults=[None, None, 'utf-8'])
class TemporaryTextFile:
    def __init__(self, txt_file, directory=None):
        if not isinstance(txt_file, TextFile):
            raise ValueError("Argument of TemporaryTextFile must be an instance of the 'TextFile' named tuple")
        self.directory = directory if directory else tempfile.gettempdir()
        self.path = os.path.join(self.directory, txt_file.file_name)
        create_text_file(self.path, txt_file.content, txt_file.encoding)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        try: os.remove(self.path)
        except: pass

#----------------------------------------------------------------------------
def check_strings_equality(str1, str2):
    if str1==str2:
        return True
    # Strings are different: some diagnostic
    min_length = min(len(str1), len(str2))
    for i in range(min_length):
        if str1[i]!=str2[i]:
            print(f"First string at position {i} has character '{repr(str1[i])}', while second has character '{repr(str2[i])}'")
            portion1 = ''.join(repr(c) for c in str1[:i+1])
            portion2 = ''.join(repr(c) for c in str2[:i+1])
            print(f"The string portions are:\n{portion1}\n{portion2}")
            break
    if len(str1)!=len(str2):
        print(f"Strings are equal up to position {min_length}, but the {'first' if len(str1)>len(str2) else 'second'} has additional characters")
    return False

#----------------------------------------------------------------------------
def textfile_content_is(file_path, encoding, expected):
    with open(file_path, 'rb') as file:
        content = file.read().decode(encoding)
        if check_strings_equality(content,expected):
            return True
        else:
            print(f'{GRAY}expected:{END}\n{expected}')
            print(f'{GRAY}actual:{END}\n{content}')
    return False

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
    def __init__(self, menu_key, title, action):
        self.menu_key = menu_key
        self.title = title
        self.action = action

    def run(self, manual_mode):
        return self.action(manual_mode)

#----------------------------------------------------------------------------
def create_tests_array(manual_mode):
    tests = [
        Test('1', "Update_no_file", test_update_no_file),
        Test('2', "Update_empty", test_update_empty),
        Test('3', "Update_no_libs", test_update_no_libs),
        Test('4', "Update_same_prj", test_update_same_prj),
        Test('5', "Update_bad_prj", test_update_bad_prj),
        Test('6', "Update_bad_lib", test_update_bad_plclib),
        Test('7', "Update_nonexistent_lib", test_update_nonexistent_lib),
        Test('8', "Update_simple", test_update_simple_prj),
        Test('a', "Update_ppjs", test_update_ppjs),
        Test('b', "Update_plcprj", test_update_plcprj)
       ]
    if manual_mode:
        tests += [
            Test('c', "Update_strato_ppjs", test_update_strato_ppjs),
            Test('d', "Update_strato_plcprj", test_update_strato_plcprj)
           ]
    return tests



#============================================================================
def test_update_no_file(manual_mode):
    ret_code = launch([exe, "update", "-v" if manual_mode else "-q"])
    return ret_code==2 # no file given should give a fatal error


#============================================================================
def test_update_empty(manual_mode):
    prj = TextFile('empty.ppjs', '')
    with TemporaryTextFile(prj) as prj_file:
        ret_code = launch([exe, "update", "-v" if manual_mode else "-q", prj_file.path])
        return ret_code==2 # empty file should give a fatal error


#============================================================================
def test_update_no_libs(manual_mode):
    prj = TextFile( 'nolibs.ppjs',
                    '<plcProject>\n'
                    '    <libraries>\n'
                    '    </libraries>\n'
                    '</plcProject>\n' )
    with TemporaryTextFile(prj) as prj_file:
        ret_code = launch([exe, "update", "-v" if manual_mode else "-q", prj_file.path])
        return ret_code==1 # no libraries should raise an issue


#============================================================================
def test_update_same_prj(manual_mode):
    prj = TextFile( 'same.ppjs',
                    '<plcProject>\n'
                    '    <libraries>\n'
                    '    </libraries>\n'
                    '</plcProject>\n',
                    'utf-16' )
    with TemporaryTextFile(prj) as prj_file:
        ret_code = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", prj_file.path, prj_file.path])
        return ret_code==2 # same output file should give a fatal error


#============================================================================
def test_update_bad_prj(manual_mode):
    prj = TextFile( 'bad.ppjs',
                    '<plcProject>\n'
                    '    <libraries>\n'
                    '        <lib link="true" name="unclosed">\n'
                    '            <![CDATA[...]]>\n'
                    '    </libraries>\n'
                    '</plcProject>\n',
                    'utf-16' )
    with TemporaryTextFile(prj) as prj_file:
        if manual_mode:
            ret_code = launch([exe, "update", "-v", prj_file.path])
            time.sleep(0.5) # Give the time to show the bad file
        else:
            ret_code = launch([exe, "update", "-q", prj_file.path])
        return ret_code==2 # bad project should give a fatal error


#============================================================================
def test_update_bad_plclib(manual_mode):
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
        def full_path(fname): return os.path.join(temp_dir, fname)
        create_text_file(full_path(bad_plclib.file_name), create_plclib_full_content(bad_plclib.file_name, bad_plclib.content), bad_plclib.encoding)
        prj_path = full_path(prj.file_name)
        create_text_file(prj_path, prj.content, prj.encoding)
        if manual_mode:
            ret_code = launch([exe, "update", "-v", prj_path])
            time.sleep(0.5) # Give the time to show the bad file
        else:
            ret_code = launch([exe, "update", "-q", prj_path])
        return ret_code==2 # bad library should give a fatal error


#============================================================================
def test_update_nonexistent_lib(manual_mode):
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
        def full_path(fname): return os.path.join(temp_dir, fname)
        create_text_file(full_path(pll1.file_name), pll1.content, pll1.encoding)
        prj_path = full_path(prj.file_name)
        create_text_file(prj_path, prj.content, prj.encoding)
        ret_code = launch([exe, "update", "-v" if manual_mode else "-q", prj_path])
        expected = prj.content.replace(f'name="{pll1.file_name}">x', f'name="{pll1.file_name}"><![CDATA[{pll1.content}]]>')
        content_equal = textfile_content_is(prj_path, prj.encoding, expected)
        return ret_code==1 # nonexistent library should raise an issue


#============================================================================
def test_update_simple_prj(manual_mode):
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
        def full_path(fname): return os.path.join(temp_dir, fname)
        create_text_file(full_path(pll1.file_name), pll1.content, pll1.encoding)
        create_text_file(full_path(pll2.file_name), pll2.content, pll2.encoding)
        create_text_file(full_path(plclib1.file_name), create_plclib_full_content(plclib1.file_name, plclib1.content), plclib1.encoding)
        create_text_file(full_path(plclib2.file_name), create_plclib_full_content(plclib2.file_name, plclib2.content), plclib2.encoding)
        prj_path = full_path(prj.file_name)
        create_text_file(prj_path, prj.content, prj.encoding)
        out_path = full_path("updated.xml")
        retcode = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", out_path, prj_path])
        expected = ( prj.content.replace(f'name="{pll1.file_name}"><![CDATA[prev content]]>', f'name="{pll1.file_name}"><![CDATA[{pll1.content}]]>')
                                .replace(f'name="{pll2.file_name}">', f'name="{pll2.file_name}"><![CDATA[{pll2.content}]]>')
                                .replace(f'name="{plclib1.file_name}">prev content', f'name="{plclib1.file_name}">{plclib1.content}')
                                .replace(f'name="{plclib2.file_name}">', f'name="{plclib2.file_name}">{plclib2.content}') )
        content_equal = textfile_content_is(out_path, prj.encoding, expected)
        if manual_mode and os.path.isfile(out_path):
            show_text_files_comparison(prj_path, out_path)
        return retcode==0 and content_equal


#============================================================================
def test_update_ppjs(manual_mode):
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
        def full_path(fname): return os.path.join(temp_dir, fname)
        libs_dir = os.path.join(temp_dir, lib_subfolder)
        os.mkdir(libs_dir) 
        create_text_file(os.path.join(libs_dir,defvar.file_name), defvar.content, defvar.encoding)
        create_text_file(os.path.join(libs_dir,iomap.file_name), iomap.content, iomap.encoding)
        prj_path = full_path(prj.file_name)
        create_text_file(prj_path, prj.content, prj.encoding)
        out_path = full_path("updated.xml")
        retcode = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", out_path, prj_path])
        expected = ( prj.content.replace('#DEFVAR#', defvar.content)
                                .replace('#IOMAP#', iomap.content) )
        content_equal = textfile_content_is(out_path, prj.encoding, expected)
        if manual_mode and os.path.isfile(out_path):
            show_text_files_comparison(prj_path, out_path)
        return retcode==0 and content_equal


#============================================================================
def test_update_plcprj(manual_mode):
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
        def full_path(fname): return os.path.join(temp_dir, fname)
        libs_dir = os.path.join(temp_dir, lib_subfolder)
        os.mkdir(libs_dir) 
        create_text_file(os.path.join(libs_dir,defvar.file_name), create_plclib_full_content(defvar.file_name, defvar.content), defvar.encoding)
        create_text_file(os.path.join(libs_dir,iomap.file_name), create_plclib_full_content(iomap.file_name, iomap.content), iomap.encoding)
        prj_path = full_path(prj.file_name)
        create_text_file(prj_path, prj.content, prj.encoding)
        out_path = full_path("updated.xml")
        retcode = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", out_path, prj_path])
        expected = ( prj.content.replace('#DEFVAR#', defvar.content)
                                .replace('#IOMAP#', iomap.content) )
        content_equal = textfile_content_is(out_path, prj.encoding, expected)
        if manual_mode and os.path.isfile(out_path):
            show_text_files_comparison(prj_path, out_path)
        return retcode==0 and content_equal


#============================================================================
def test_update_strato_ppjs(manual_mode):
    prj = os.path.expandvars("%UserProfile%/Macotec/Machines/m32-Strato/sde/PLC/LogicLab/LogicLab.ppjs")
    out = "~updated.xml"
    with tempfile.TemporaryDirectory() as temp_dir:
        out = os.path.join(temp_dir, out)
        retcode = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", out, prj])
        if manual_mode and os.path.isfile(out):
            show_text_files_comparison(prj, out)
        return True


#============================================================================
def test_update_strato_plcprj(manual_mode):
    prj = os.path.expandvars("%UserProfile%/Macotec/Machines/m32-Strato/sde/PLC/LogicLab/LogicLab-plclib.plcprj")
    out = "~updated.xml"
    with tempfile.TemporaryDirectory() as temp_dir:
        out = os.path.join(temp_dir, out)
        retcode = launch([exe, "update", "-vF" if manual_mode else "-qF", "-o", out, prj])
        if manual_mode and os.path.isfile(out):
            show_text_files_comparison(prj, out)
        return True



#----------------------------------------------------------------------------
def run_tests(tests_array, manual_mode):
    tests_results = []
    fails = 0
    for test in tests_array:
        print(f'\n-------------- {GRAY}test {YELLOW}{test.title}{END}')
        if test.run(manual_mode) :
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
def ask_what_to_do(tests_array, manual_mode):
    def print_menu(tests_array):
        print("\n____________________________________")
        for test in tests_array:
            print(f"{YELLOW}[{test.menu_key}]{END}{test.title}", end=' ')
        print(f"{YELLOW}[q]{END}Quit")
    def print_selected_entry(key,name):
        print(f'{CYAN}[{key}]{BLUE}{name}{END}')
    def get_test(key_char, tests_array):
        return [test for test in tests_array if test.menu_key==key_char]

    print_menu(tests_array)
    key_char = wait_for_keypress()
    if selected:=get_test(key_char, tests_array):
        for test in selected:
            print_selected_entry(key_char,test.title)
            if test.run(manual_mode) :
                print(f'{GREEN}[passed] {test.title}{END}')
            else:
                print(f'{RED}[failed] {test.title}{END}')
    else:
        match key_char:
            case 'q':
                print_selected_entry(key_char,'Quit')
                return False
            case _:
                print(f"{RED}'{key_char}' is not a valid option{END}")
    return True


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)
    if not os.path.isfile(exe):
        print(f"{RED}{exe} not found!{END}")
        pause()
        sys.exit(1)
    manual_mode = is_manual_mode()
    print(f'{exe} ({"manual" if manual_mode else "auto"})')
    tests = create_tests_array(manual_mode);
    if manual_mode:
        while ask_what_to_do(tests, manual_mode): pass
    else:
        fails = run_tests(tests, manual_mode)
        if fails>0:
            print(f"\n{RED}Had {fails} fails!{END}")
            pause()
        else:
            print(f"\n{GREEN}All {len(tests)} tests passed{END}")
            print("Closing...")
            time.sleep(3)
        return fails

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
