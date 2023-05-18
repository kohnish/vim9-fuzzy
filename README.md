Vim9-fuzzy
=========

A vim plugin for finding files with approximate string matching.
No vim language binding dependencies, use job-start API to avoid blocking vim

ToDo
----
 - Reduce hard coding
 - Remove the default rg executables dependency
 - Add backtrace
 - Add tests
 - Improve fuzzy algorithm
 - Allow custom keybinding
 - UTF-8 support
 - Test on Windows

Runtime requirements
--------------------
 - Vim with vim9 script support
 - Rg (Only for the default list command, can be overridden by Vim9fuzzy_user_list_func)

Usage
-----
Installation:
```sh
mkdir -p ~/.vim/pack/plugins/opt
git clone https://github.com/kohnish/vim9-fuzzy ~/.vim/pack/plugins/opt/vim9-fuzzy
# For Linux 
curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v0.7/vim9-fuzzy-linux-x86-64 -o ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy 
chmod +x ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy
# For Windows
curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v0.7/vim9-fuzzy-win-x86-64 -o ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy.exe
chmod +x ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy.exe
# Or see the build section for compiling locally


```
Configuration
```vim
## Required configurations

# Enable vim9-fuzzy
packadd! vim9-fuzzy

# Root path to search from (No default yet)
var proj_dir = getcwd()
var git_root = system("git rev-parse --show-toplevel | tr -d '\n'")
if len(git_root) > 0
    proj_dir = git_root
endif
g:vim9_fuzzy_proj_dir = proj_dir

# Vim9-fuzzy Keymap (No defaults)
# Search by only file name.
noremap <C-f> :Vim9FuzzyFile<CR>
# Search by full path
noremap <C-p> :Vim9FuzzyPath<CR>
# Search in last opened files via vim9-fuzzy.
noremap <C-e> :Vim9FuzzyMru<CR>


# Optional settings
# Override defaut list cmd
g:vim9fuzzy_user_list_func = true
def g:Vim9fuzzy_user_list_func(root_dir: string, target_dir: string): string
    var git_exe = exepath("git")
    var dir = target_dir
    if dir == ""
        dir = root_dir
    endif
    var is_in_git_dir = system("cd " .. dir .. " && " .. git_exe .. " rev-parse --is-inside-work-tree")
    var in_git_dir = v:shell_error == 0
    var is_in_ignore_dir = system("git check-ignore " .. dir)
    var in_ignore_dir = v:shell_error == 0
    if in_git_dir && !in_ignore_dir
        return git_exe .. " ls-files --full-name " .. dir .. " | egrep -v '^.*(\.png|\.jpg)$' && " .. git_exe .. " clean --dry-run -d | awk '{print $3}'"
    endif
    return "find " .. dir .. " -type f -maxdepth 5"
enddef

# Path for keeping most recently used files (Default here)
g:vim9_fuzzy_mru_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/mru"

# For fuzzy yank history search
noremap <C-y> :Vim9FuzzyYank<CR>
# Enable yank hook (Default false)
g:vim9_fuzzy_yank_enabled = true
# The path for keeping yank histories (Defaulting here)
g:vim9_fuzzy_yank_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/yank"

```

Build requirements
------------------
 - CMake
 - pkg-config
 - C compiler (GCC / Clang)
 - C++ compiler (Optional for test)  
  

Build (static build(needs internet access))
```shell
mkdir build
cd build
cmake -DBUILD_STATIC=ON ..
make -j`nproc`
make install/strip
```

Build (dynamic build(Fedora))
```shell
mkdir build
cd build
sudo dnf install -y libuv-devel
cmake ..
make -j`nproc`
make install/strip
```
