Vim9-fuzzy
=========

Yet another fuzzy search plugin for vim with vim9script support.
Fuzzy matching for files, mru, yanked texts are supported.
No vim language binding dependencies, use job-start API to avoid blocking vim

ToDo
----
 - UTF-8 support
 - Add backtrace
 - Add tests
 - Reduce hard coding
 - Improve fuzzy algorithm
 - Remove the default rg executables dependency
 - Test on Windows

Runtime requirements
--------------------
 - Vim with vim9script support
 - Rg (Only for the default list command, can be overridden by Vim9_fuzzy_user_list_func)

Usage
-----
Installation:
```sh
mkdir -p ~/.vim/pack/plugins/opt
git clone https://github.com/kohnish/vim9-fuzzy ~/.vim/pack/plugins/opt/vim9-fuzzy
add `packadd! vim9-fuzzy` in your vimrc

# For platforms with libuv-devel in the library path
:Vim9fuzzyMake

# For Linux, there is prebuilt binary
:Vim9fuzzyMake linux-download

# For MacOS x86, there is prebuilt binary
:Vim9fuzzyMake macos-download

# For Windows, there is prebuilt binary (untested)
:Vim9fuzzyMake win-download

# Or just compile static binary and install. (Check the build requirement for details)
:Vim9fuzzyMake online-build
```

Optional Configuration
```vim
# Vim9-fuzzy Keymap (No defaults)
# Search by only file name.
noremap <C-f> :Vim9FuzzyFile<CR>
# Search by full path
noremap <C-p> :Vim9FuzzyPath<CR>
# Search in last opened files via vim9-fuzzy.
noremap <C-e> :Vim9FuzzyMru<CR>
# Search in the directory of the current dir
noremap <C-k> :Vim9FuzzyPwdFile<CR>
# Exact match grep (default to rg, not so fast, just use vim grep for big projects)
noremap <C-a> :Vim9FuzzyGrep<CR>
# Or limit the directory of the current file
noremap <C-g> :Vim9FuzzyPwdGrep<CR>

# Enable MRU for all opened files (default is only for files opened via vim9fuzzy window)
g:vim9_fuzzy_enable_global_mru = true

# Path for keeping most recently used files (Default here)
g:vim9_fuzzy_mru_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/mru"

# Enable preview window
g:vim9_fuzzy_enable_preview = true

# Modify preview window height
g:vim9_fuzzy_file_preview_height = 25
g:vim9_fuzzy_yank_preview_height = 10

# Search window height
g:vim9_fuzzy_win_height = 15

# Override select keys
g:vim9_fuzzy_edit_key = "\<CR>"
g:vim9_fuzzy_vsplit_key = "\<C-v>"
g:vim9_fuzzy_tabedit_key = "\<C-t>"
g:vim9_fuzzy_yank_paste_key = "\<CR>"
g:vim9_fuzzy_yank_only_key = "\<C-t>"

# For fuzzy yank history search (No windows support)
# Enable yank hook (Default false)
g:vim9_fuzzy_yank_enabled = true
noremap <C-y> :Vim9FuzzyYank<CR>
# The path for keeping yank histories (Defaulting here)
g:vim9_fuzzy_yank_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/yank"

# Root path to search from statically (Default to the current dir)
var proj_dir = getcwd()
var git_root = system("git rev-parse --show-toplevel | tr -d '\n'")
if len(git_root) > 0
    proj_dir = git_root
endif
g:vim9_fuzzy_proj_dir = proj_dir


# Overriding list and grep by setting callbacks

# Helper functions
def GetProjRoot(base_dir: string): string
    var root_dir = trim(system(g_git_cmd .. " -C " .. base_dir .. " rev-parse --show-toplevel 2>/dev/null"))
    if empty(root_dir)
        root_dir = getcwd()
    endif
    return root_dir
enddef

def TargetDir(root_dir: string, target_dir: string): string
    var dir = target_dir
    if dir == ""
        dir = root_dir
    endif
    return dir
enddef

def IsInGitDir(dir: string): bool
    var is_in_git_dir = trim(system(g_git_cmd .. " -C " .. dir .. " rev-parse --is-inside-work-tree 2>/dev/null"))
    var in_git_dir = v:shell_error == 0
    var is_in_ignore_dir = trim(system(g_git_cmd .. " check-ignore " .. dir .. " 2>/dev/null"))
    var in_ignore_dir = v:shell_error == 0
    return in_git_dir && !in_ignore_dir
enddef

var g_last_git_dir = ""
def WasInGitDir(dir: string): bool
    if dir == g_last_git_dir
        return true
    endif
    if IsInGitDir(dir)
        g_last_git_dir = dir
        return true
    endif
    return false
enddef

def ListCmd(root_dir: string, target_dir: string): dict<any>
    var ignore_suffix = "flac|mp4|webm|png|jpg|jpeg|gz|zip|7z|xz|pdf|apk|dmg|pkg|apk|exe"
    var dir = TargetDir(root_dir, target_dir)
    if WasInGitDir(dir)
        return {
            "trim_target_dir": true,
            "cmd": "cd " .. dir .. " && rg --files | rg -v -e '^.*\.(" .. ignore_suffix .. ")$'"
        }
    endif
    return {
        "trim_target_dir": false,
        "cmd": "rg --files ---max-depth 3 " .. dir .. " | rg -v -e '^.*\.(" .. ignore_suffix .. ")$'"
    }
enddef

def GrepCmd(keyword: string, root_dir: string, target_dir: string): dict<any>
    var dir = TargetDir(root_dir, target_dir)
    if WasInGitDir(dir)
        return {
            "trim_target_dir": true,
            "cmd": g_git_cmd .. " -C " .. dir .. " grep -n '" .. keyword .. "'"
        }
    endif
    return {
        "trim_target_dir": false,
        "cmd":  "rg --max-depth=2 --max-filesize 1M --color=never -Hn --no-heading '" .. keyword .. "' " .. dir
    }
enddef

# Override proj root find func
g:Vim9_fuzzy_get_proj_root_func = () => GetProjRoot(getcwd())

# Override list command string
g:Vim9_fuzzy_list_func = (root_dir, target_dir) => ListCmd(root_dir, target_dir)

# Override grep command, make sure the function is not blocking, unlike list command, it's called on every input
g:Vim9_fuzzy_grep_func = (keyword, root_dir, target_dir) => GrepCmd(keyword, root_dir, target_dir)

```

Build requirements
------------------
 - CMake
 - pkg-config
 - make / ninja
 - C compiler (GCC / Clang)
 - C++ compiler (Optional for test)  

Build (static build(needs internet access))
```shell
cmake -Bbuild -DBUILD_STATIC=ON
cmake --build build --target install/strip
```

Build (dynamic build(Fedora))
```shell
sudo dnf install -y libuv-devel
cmake -Bbuild
cmake --build build --target install/strip
```

Build (dynamic build(MSYS)
```shell
cmake -GNinja -Bbuild -DBUILD_STATIC=OFF -DUSE_LOCAL_JSMN=OFF
cmake --build build --target install/strip
```

