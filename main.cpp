#define UNICODE
#define _UNICODE

#include <windows.h>

#include <string.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

#include "mingw-unicode.h"

namespace calibre4mhl {
//Calibre command-line tools dir
std::wstring calibre_bin = L"";

//transform input Utf8 string into Windows Unicode wstring
std::wstring convert_from_utf8_to_utf16(const std::string& str) {
    std::wstring converted_string;
    int required_size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    if (required_size > 0) {
        std::vector<wchar_t> buffer(required_size);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], required_size);
        converted_string.assign(buffer.begin(), buffer.end() - 1);
    }

    return converted_string;
}

//transform input Windows Ansi string into Windows Unicode wstring
std::wstring convert_from_ansi_to_utf16(const std::string& str) {
    std::wstring converted_string;
    int required_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
    if (required_size > 0) {
        std::vector<wchar_t> buffer(required_size);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &buffer[0], required_size);
        converted_string.assign(buffer.begin(), buffer.end() - 1);
    }

    return converted_string;
}

//transform Windows Unicode wstring into Utf8 string
std::string convert_from_utf16_to_utf8(const std::wstring& wstr) {
    std::string converted_string;
    int required_size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if(required_size > 0) {
        std::vector<char> buffer(required_size);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], required_size, 0, 0);
        converted_string.assign(buffer.begin(), buffer.end() - 1);
    }
    return converted_string;
}

//launch with args and append a line to commandline
unsigned long launch(const std::vector<std::wstring>& args, const std::wstring& append_line = L"") {
	unsigned long exitCode;

	std::wstring cmd = (std::wstring)_wgetenv (L"COMSPEC") + L" /U /C \"";
	wchar_t* cmdc;

    for(size_t i = 0; i < args.size(); i++)
        if (args[i].find(L" ") != std::wstring::npos)
            cmd += L" \"" + args[i] + L"\"";
        else
            cmd += L" " + args[i];
    cmd += L" " + append_line + L"\"";

	// Set up PROCESS_INFORMATION structure
	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Declare process info
	STARTUPINFO siStartInfo;

	// Set up STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);

	cmdc = new wchar_t [cmd.length() + 1];
	wcscpy(cmdc, cmd.c_str());

	if (!CreateProcess(NULL,
		cmdc,                       // command line
		NULL,          				// process security attributes
		NULL,          				// primary thread security attributes
		TRUE,                    	// handles are inherited
		CREATE_UNICODE_ENVIRONMENT, // creation flags
		NULL,          				// use parent's environment
		NULL,          				// use parent's current directory
		&siStartInfo,  				// STARTUPINFO pointer
		&piProcInfo))  				// receives PROCESS_INFORMATION
	{
		delete[] cmdc;
		throw 3;
	}
	// Successfully created the process.  Wait for it to finish.
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);

	// Get the exit code.
	if (!GetExitCodeProcess(piProcInfo.hProcess, &exitCode)) {
		delete[] cmdc;
		throw 4;
	}

	// Close handles to the child process and its primary thread
	CloseHandle(piProcInfo.hThread);
	CloseHandle(piProcInfo.hProcess);

	delete[] cmdc;
	return exitCode;
}

//launch command with args and save output to out
unsigned long launch(const std::vector<std::wstring>& args, std::string& out) {
	unsigned long exitCode;

	std::wstring cmd = (std::wstring)_wgetenv (L"COMSPEC") + L" /U /C \"";
	wchar_t* cmdc;

    for(size_t i = 0; i < args.size(); i++)
        if (args[i].find(L" ") != std::wstring::npos)
            cmd += L" \"" + args[i] + L"\"";
        else
            cmd += L" " + args[i];
    cmd += L"\"";

	// Declare handle
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;

    // Create security attributes
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		throw 1;

	// Ensure the read handle to the pipe for STDOUT is not inherited
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		throw 2;

	// Set up members of the PROCESS_INFORMATION structure
	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Declare process info
	STARTUPINFO siStartInfo;

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	cmdc = new wchar_t [cmd.length() + 1];
	wcscpy(cmdc, cmd.c_str());

	if (!CreateProcess(NULL,
		cmdc,                       // command line
		NULL,          				// process security attributes
		NULL,          				// primary thread security attributes
		TRUE,                    	// handles are inherited
		CREATE_UNICODE_ENVIRONMENT, // creation flags
		NULL,          				// use parent's environment
		NULL,          				// use parent's current directory
		&siStartInfo,  				// STARTUPINFO pointer
		&piProcInfo))  				// receives PROCESS_INFORMATION
	{
		delete[] cmdc;
		throw 3;
	}
	// Successfully created the process.  Wait for it to finish.
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);

	// Get the exit code.
	if (!GetExitCodeProcess(piProcInfo.hProcess, &exitCode)) {
		delete[] cmdc;
		throw 4;
	}

	// Close handles to the child process and its primary thread
	CloseHandle(piProcInfo.hThread);
	CloseHandle(piProcInfo.hProcess);

	// Read output from the child process's pipe for STDOUT
	// Stop when there is no more data.


	// Close the write end of the pipe before reading from the
	// read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the
	// data the child process has already written to it.
	if(!CloseHandle(g_hChildStd_OUT_Wr)){
		delete[] cmdc;
		throw 5;
	}
	char chBuf[4097];
	unsigned long dwRead = 1;
	out="";
	while (dwRead != 0 && ReadFile(g_hChildStd_OUT_Rd, chBuf, 4096, &dwRead, NULL))
    {
        chBuf[4096] = '\0';
        out += chBuf;
    }


	delete[] cmdc;
	return exitCode;
}

//remove leading '<field>: '
std::string remove_field_name(const std::string& str) {
    std::string out = str;
    size_t i = out.find(':');
    if (i != std::string::npos) {
        out.erase(0, i + 1);
        if (out[0] == ' ')
            out.erase(0, 1);
    }
    return out;
}

//remove trailing '#...'
std::string remove_number(const std::string& str) {
    std::string out = str;
    size_t i = out.rfind('#');
    if (i != std::string::npos) {
        out.erase(i, std::string::npos);
        if (out[out.length() - 1] == ' ')
            out.erase(out.length() - 1);
    }
    return out;
}

//get series number from '<series> #<number>'
std::string get_number(const std::string& str) {
    std::string out = str;
    size_t i;
    for(i = out.length() - 1; i >= 0 && out[i] != '#'; i--);
    if (i >= 0)
        out.erase(0, i + 1);
    else
        out ="";
    return out;
}

//parse full filename into <parent_path>/<stem><extension>
void parse_full_filename(const std::wstring& full_filename, std::wstring& parent_path, std::wstring& stem, std::wstring& extension) {
    std::wstring filename = full_filename;

    for(size_t i=0; i < filename.length(); i++)
        if (filename[i] == L'/')
            filename[i] = L'\\';

    size_t found = filename.rfind(L'\\');
	if (found == std::wstring::npos) {
        parent_path = L".";
//      filename = full_filename;
        //already equal to full_filename
	}
    else {
        parent_path = full_filename.substr(0, found);
        filename = full_filename.substr(found + 1, std::wstring::npos);
    }

    found = filename.rfind(L'.');
    if (found == std::wstring::npos) {
        extension = L"";
        stem = filename;
	}
    else {
        stem = filename.substr(0, found);
        extension = filename.substr(found, std::wstring::npos);
    }
}

//transform input '<path>\<filename>.<ext>' string into '<path>\<filename> <i>.<ext>'
std::wstring append_i(const std::wstring& file, const unsigned i) {
    std::wostringstream ss;
    std::wstring path, stem, extension;

    parse_full_filename(file, path, stem, extension);

    ss << path << L'\\' << stem << L' ' << i << extension;

    return ss.str();
}

//check if file exists
bool file_exists(const std::wstring& name) {
    if (FILE *file = _wfopen(name.c_str(), L"r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

bool dir_exists(const std::string& dirName_in) {
  unsigned long ftype = GetFileAttributes(convert_from_utf8_to_utf16(dirName_in).c_str());
  if (ftype == INVALID_FILE_ATTRIBUTES)
    return false;  //something is wrong with your path!

  if (ftype & FILE_ATTRIBUTE_DIRECTORY)
    return true;   // this is a directory!

  return false;    // this is not a directory!
}

std::string get_full_path(const std::string& partialPath )
{
   wchar_t full[_MAX_PATH];
   if( _wfullpath( full, convert_from_utf8_to_utf16(partialPath).c_str(), _MAX_PATH ) != NULL )
      return convert_from_utf16_to_utf8(full);
   else
      return "";
}

bool is_ascii_string(const std::string& str) {
    for(size_t i = 0; i < str.length(); i++)
        if (!isascii(str[i]))
            return false;

    return true;
}

//generate not existing filename '<system_tmp_dir>\<filename> <random_integer>.<ext>'
//based upon given '<path>\<filename>.<ext>' string
std::wstring get_temporary_name(const std::wstring& file) {
    std::wstring tmpdir = _wgetenv (L"TMP");

    std::wstring path, stem, extension;

    parse_full_filename(file, path, stem, extension);

    std::wstring tmpfilename = tmpdir + L"\\" + stem + extension;
    size_t i = std::rand();
    for(; file_exists(append_i(tmpfilename, i)); i = std::rand());
    tmpfilename = append_i(tmpfilename, i);

    return tmpfilename;
}

//launch `ebook-convert <file1> <file2>`
unsigned long convert_file(const std::wstring& file1, const std::wstring& file2, const std::wstring& append = L"") {

    std::vector<std::wstring> args;

    args.push_back(calibre_bin + L"ebook-convert");
    args.push_back(file1);
    args.push_back(file2);

    return launch(args, append);
}

//launch `ebook-meta <file1>` to print metada to stdout
unsigned long print_meta(const std::wstring& file) {
    std::vector<std::wstring> args;

    args.push_back(calibre_bin + L"ebook-meta");
    args.push_back(file);

    return launch(args);
}

//launch `ebook-meta <file1> <field> <value> >nul:`
//to set respective <file1> metadata field and suppress output
unsigned long set_meta(const std::wstring& file, const std::wstring& field, const std::wstring& value) {
    std::vector<std::wstring> args;

    args.push_back(calibre_bin + L"ebook-meta");
    args.push_back(file);
    args.push_back(field);
    args.push_back(value);
    args.push_back(L">nul:");

    return launch(args);
}

unsigned long copy_file(const std::wstring& file1, const std::wstring& file2) {
    std::vector<std::wstring> args;

    args.push_back(L"copy");
    args.push_back(L"/Y");
    args.push_back(file1);
    args.push_back(file2);
    args.push_back(L">nul:");

    return launch(args);
}

unsigned long delete_file(const std::wstring& file) {
    std::vector<std::wstring> args;

    args.push_back(L"del");
    args.push_back(L"/F");
    args.push_back(file);
    args.push_back(L">nul:");

    return launch(args);
}

//same as print_meta, except for redirecting stdout to a variable,
//then parsing it, storing fields "Title" and "Series" in
//respective title and series arguments, setting has_series
//to TRUE if "Series" field is present, FALSE otherwise,
unsigned long set_meta(const std::wstring& file, std::string& title, std::string& series, bool& has_series) {
    unsigned long exit_code;

    std::vector<std::wstring> args;
    std::string out;

    args.push_back(calibre_bin + L"ebook-meta");
    args.push_back(file);

    //launch ebook-meta file
    exit_code = launch(args, out);

    if (exit_code != 0)
        return exit_code;

    std::istringstream buff (out);

    std::string s;
    has_series = false;
    while (std::getline(buff, s)) {
        if (s.compare(0, 5, "Title") == 0)
            title = remove_field_name(s);
        if (s.compare(0, 6, "Series") == 0) {
            series = remove_field_name(s);
            has_series = true;
        }

        //it is possible, in theory, for comments to have a line starting
        //with "Title" or "Series", so to avoid mistakenly setting
        //title/series/has_series, we should stop parsing after
        //reaching the Comments field, which is printed last
        if (s.compare(0, 8, "Comments") == 0)
            break;
    }

    return 0;
}

//check whether given char is a valid extension char
bool is_valid_ext(const char& c) {
    std::string str = " ";
    str[0] = c;

    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >=  '0' && c <= '9') ||
            (str.find_first_of("!#$%&'()+,-;=@[]^_`{¦}~") == 0);
}

//check whether given string is a valid extension
bool is_valid_ext(const std::string& ext) {
    for(size_t i = 0; i < ext.length(); i++)
        if (!is_valid_ext(ext[i]))
                return false;

     return true;
}

//convert string/wstring to lower case
template <typename T>
    T to_lower(const T& str) {
        std::locale loc;
        T out = str;
        for(size_t i = 0; i < out.length(); i++)
            out[i] = std::tolower(out[i], loc);

        return out;
    }

//this class parses config file lines and stores parsed settings
class config {
private:
    std::string line, field, value;
    void remove_comments(void);
    std::string parse_line(void);
    void remove_leading_and_trailing_spaces(void);
public:
    bool ext_set, skip_meta_check_set, command_line_args_set, calibre_dir_set, skip_meta_check;
    std::string ext, command_line_args, calibre_dir;

    config (void) {
        line = "";
        field = "";
        value = "";
        ext_set = false;
        skip_meta_check_set = false;
        command_line_args_set = false;
        calibre_dir_set = false;
        skip_meta_check = false;
        ext = "";
        command_line_args = "";
        calibre_dir = "";
    }

    std::string add_line(const std::string&);
};

std::string config::add_line(const std::string& in) {
    line = in;
    field = "";
    value = "";

    std::string str = "";

    remove_comments();
    if (line.empty())
        return "";
    str = parse_line();
    if (!str.empty())
        return str;
    remove_leading_and_trailing_spaces();

    std::string loc=field;

    if (field == "skip_meta_check") {
        if (skip_meta_check_set)
            return "Re-definition of already set field \"skip_meta_check\"";
        if (to_lower(value) == "true")
            skip_meta_check = true;
        else if (to_lower(value) == "false")
            skip_meta_check = false;
        else
            return "\"skip_meta_check\" field value must be: true or false";
        skip_meta_check_set = true;
    } else if (field == "extension") {
        if (ext_set)
            return "Re-definition of already set field \"extension\"";
        if (!is_valid_ext(value))
            return "\"extension\" field value must be a valid extension";
        ext = value;
        ext_set = true;
    } else if (field == "args") {
        if (command_line_args_set)
            return "Re-definition of already set field \"args\"";
        command_line_args = value;
        command_line_args_set = true;
    } else if (field == "calibre_dir") {
        if (calibre_dir_set)
            return "Re-definition of already set field \"calibre_dir\"";
        if (value.length() > 1 && value[0] == '\"' && value[value.length() - 1] == '\"')
            value = value.substr(1, value.length() - 2);
        if (value.empty()) {
            calibre_dir = "";
            calibre_dir_set = true;
            return "";
        }
        if (!dir_exists(value))
            return "Wrong dir name/dir doesn't exist";
        if (!is_ascii_string(get_full_path(value)))
            return "Absolute path contains non-ASCII characters. Calibre's command-line tools won't work properly";
        calibre_dir = value;
        calibre_dir_set = true;
    } else
        return (std::string)"Wrong field name: " + field;

    return "";
}

void config::remove_comments(void) {
    size_t i = 0;
    for(bool quotes_open = false; line[i] != ':' || quotes_open; i++)
        if (line[i] == '\"')
            quotes_open = !quotes_open;
    if (i < line.length())
        line.erase(i, std::string::npos);
}

std::string config::parse_line(void) {
    size_t i = line.find('=');

    if (i == std::string::npos)
        return "Wrong line format";

    field = line.substr(0, i);
    value = line.substr(i + 1, std::string::npos);

    if (std::count(value.begin(), value.end(), '\"') % 2)
        return "Unmatched quotation mark (\")";

    line = "";

    return "";
}

void config::remove_leading_and_trailing_spaces(void) {
    if (!field.empty()) {
        size_t i = 1;
        if (field[0] == ' ') {
            for(; i < field.length() && field[i] == ' '; i++);
            field.erase(0, i);
        }
        i = 1;
        if (field[field.length() - 1] == ' ') {
            for(; i < field.length() && field[field.length() - i - 1] == ' '; i++);
            field.erase(field.length() - i, std::string::npos);
        }
    }

    if (!value.empty()) {
        size_t i = 1;
        if (value[0] == ' ') {
            for(; i < value.length() && value[i] == ' '; i++);
            value.erase(0, i);
        }
        i = 1;
        if (value[value.length() - 1] == ' ') {
            for(; i < value.length() && value[value.length() - i - 1] == ' '; i++);
            value.erase(value.length() - i, std::string::npos);
        }
    }
}

//get full title = <Series> #<Number> - <Title>
std::vector<std::wstring> GetFullTitle(const std::wstring& file) {
    std::vector<std::wstring> out;
    std::string title, series;
    bool has_series;

    if (set_meta(file, title, series, has_series) != 0) {
        out.push_back(L"");
        out.push_back(L"Failed to get file meta: " + file);
        return out;
    }

    //[test ebook-meta output encoding
    bool isutf8 = false;

    //try default windows ANSI
    std::wstring tmpfile = get_temporary_name(file);
    if (copy_file(file, tmpfile) != 0) {
        delete_file(tmpfile);
        out.push_back(L"");
        out.push_back(L"Failed to create source file temporary copy");
        return out;
    }

    set_meta(tmpfile, L"--title", convert_from_ansi_to_utf16(title));
    set_meta(tmpfile, L"--series", convert_from_ansi_to_utf16(remove_number(series)));
    set_meta(tmpfile, L"--index", convert_from_ansi_to_utf16(get_number(series)));

    std::string title2, series2;
    bool has_series2;

    if (set_meta(tmpfile, title2, series2, has_series2) != 0) {
        delete_file(tmpfile);
        out.push_back(L"");
        out.push_back(L"Failed to get file meta: " + tmpfile);
        return out;
    }

    if (title != title2 || series != series2) {
        //try utf8
        set_meta(tmpfile, L"--title", convert_from_utf8_to_utf16(title));
        set_meta(tmpfile, L"--series", convert_from_utf8_to_utf16(remove_number(series)));
        set_meta(tmpfile, L"--index", convert_from_utf8_to_utf16(get_number(series)));

        if (set_meta(tmpfile, title2, series2, has_series2) != 0) {
            delete_file(tmpfile);
            out.push_back(L"");
            out.push_back(L"Failed to get file meta: " + tmpfile);
            return out;
        }

        if (title != title2 || series != series2) {
            out.push_back(L"");
            out.push_back(L"Error: cannot guess ebook-meta output encoding");
            delete_file(tmpfile);
            return out;
        }

        isutf8 = true;
    }

    delete_file(tmpfile);

    //]

    std::wstring titlew, seriesw;

    if (isutf8) {
        titlew = convert_from_utf8_to_utf16(title);
        seriesw = convert_from_utf8_to_utf16(series);
    }
    else {
        titlew = convert_from_ansi_to_utf16(title);
        seriesw = convert_from_ansi_to_utf16(series);
    }

    out.push_back(seriesw + L" - " + titlew);

    return out;
}
}//calibre4mhl namespace

using namespace calibre4mhl;

int wmain(int argc, wchar_t* argv[]) {
    std::srand((unsigned)std::time(NULL));
    config config;

    std::ifstream configf ("config.txt");
    if (configf.is_open()) {
        std::cout << "config.txt found. Trying to parse\n";
        std::string line;
        unsigned i = 0;
        while (getline(configf, line)) {
            i++;
            //remove UTF-8 BOM if present
            if ((i == 1) && (line.length() >= 3) &&
                (line[0] == (char)0xEF) && (line[1] == (char)0xBB) && (line[2] == (char)0xBF))
                    line.erase(0, 3);
            std::string str = config.add_line(line);
            if (!str.empty()) {
                std::cerr << "Error parsing line #" << i << ": " << str << '\n';
                return 1;
            }
        }
        configf.close();

        if (config.ext_set)
            std::cout << "Parsed: extension = " << config.ext << '\n';
        if (config.skip_meta_check_set) {
            if (config.skip_meta_check)
                std::cout << "Parsed: skip_meta_check = true\n";
            else
                std::cout << "Parsed: skip_meta_check = false\n";
        }
        if (config.command_line_args_set)
            std::cout << "Parsed: args = " << config.command_line_args << '\n';
        if (config.calibre_dir_set) {
            std::cout << "Parsed: calibre_dir = " << config.calibre_dir << '\n';
            if (!config.calibre_dir.empty())
                calibre_bin = convert_from_utf8_to_utf16(config.calibre_dir) + L"\\";
        }
        if (!config.ext_set)
            std::cout << "\"extension\" field not found, falling back to default value: extension =\n";
        if (!config.skip_meta_check_set)
            std::cout << "\"skip_meta_check\" field not found, falling back to default value: skip_meta_check = false\n";
        if (!config.command_line_args_set)
            std::cout << "\"args\" field not found, falling back to default value: args =\n";
        if (!config.calibre_dir_set)
            std::cout << "\"calibre_dir\" field not found, falling back to default value: calibre_dir =\n";
    } else
        std::cout << "config.txt not found, falling back to default settings:\nextension =\nskip_meta_check = false\n\n";

	if (argc < 3) {
		std::cerr << "Not enough arguments\n";
		return 1;
	} else { // if we got enough parameters...
		std::wstring file1 = argv[1], file2 = argv[2];

		std::wstring path, stem, extension;
            parse_full_filename(file2, path, stem, extension);

        if (config.ext_set && !config.ext.empty()) {
            extension = (std::wstring)L"." + convert_from_utf8_to_utf16(config.ext);
            file2 = path + L"\\" + stem + extension;
        }

		if (file_exists(file2)) {
            int i;
            for(i=1; file_exists (append_i(file2, i)); i++);
            file2 = append_i(file2, i);
        }

        extension = to_lower(extension);

        if (!(config.skip_meta_check_set && config.skip_meta_check) &&
            (extension == L".mobi" || extension == L".azw3"  || extension == L".lit" ||
             extension == L".lrf"  || extension == L".htmlz" || extension == L".pdf" ||
             extension == L".snb"  || extension == L".rtf"   || extension == L".txtz")) {
            std::cout << "\"skip_meta_check\" set to false. Format " << convert_from_utf16_to_utf8(extension.substr(1, std::string::npos)) << " does not support \"Series\" metadata field.\n";

            std::cout << "\nInput file metadata\n\n";

            //launch ebook-meta file1
            print_meta(file1);

            std::string title, series;
            bool has_series;

            if (set_meta(file1, title, series, has_series) != 0) {
                std::cerr << "Failed to get file meta: " << convert_from_utf16_to_utf8(file1) << '\n';
                return 1;
            }

            if (has_series) {
                std::cout << "\nSource file has \"Series\" metadata. append_ing series name to destination file title\n";

                std::vector<std::wstring> out;

                out = GetFullTitle(file1);

                if (out.size() > 1) {
                    std::cerr << convert_from_utf16_to_utf8(out[1]) << '\n';
                    return 1;
                }

                std::cout << "launching conversion\n";

                if (convert_file(file1, file2, L"--title=\"" + out[0] + L"\" " + convert_from_utf8_to_utf16(config.command_line_args)) != 0) {
                    std::cerr << "Conversion failed\n\n";
                    return 1;
                }
            } else {
                std::cout << "\nSource file has no \"Series\" metadata. No need to append series name to destination file title\n";

                std::cout << "launching conversion\n";

                if (convert_file(file1, file2, convert_from_utf8_to_utf16(config.command_line_args)) != 0) {
                    std::cerr << "Conversion failed\n\n";
                    return 1;
                }
            }

            return 0;
        }

        std::cout << "launching conversion\n";

        if (convert_file(file1, file2, convert_from_utf8_to_utf16(config.command_line_args)) != 0) {
            std::cerr << "Conversion failed\n\n";
            return 1;
        }

        if (config.skip_meta_check_set && config.skip_meta_check) {
            std::cout << "\"skip_meta_check\" set to true. Quitting...\n";
            return 0;
        }

        std::cout << '\n' << "Input file metadata\n\n";

        //launch ebook-meta file1
        print_meta(file1);

        std::string title, series, title2, series2;
        bool has_series, has_series2;

        if (set_meta(file1, title, series, has_series) != 0) {
            std::cerr << "Failed to get file meta: " << convert_from_utf16_to_utf8(file1) << '\n';
            return 1;
        }

        if (has_series) {
            if (set_meta(file2, title2, series2, has_series2) != 0) {
                std::cerr << "Failed to get file meta: " << convert_from_utf16_to_utf8(file2) << '\n';
                return 1;
            }
            if (has_series2)
                std::cout << "\nDestination file format supports \"Series\" metadata. No need to append series name to destination file title\n";
            else
                std::cout << "\nSource file has \"Series\" metadata, while destination file format does not support one. append_ing series name to destination file title\n";
        }
        else
            std::cout << "\nSource file has no \"Series\" metadata. No need to append series name to destination file title\n";

        if (has_series && !has_series2) {
            std::vector<std::wstring> out;

            out = GetFullTitle(file1);

            if (out.size() > 1) {
                std::cerr << convert_from_utf16_to_utf8(out[1]) << '\n';
                return 1;
            }

            set_meta(file2, L"--title", out[0]);
        }
	}

	return 0;
}
