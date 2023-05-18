Vim9-fuzzy
=========

A vim plugin for finding files with approximate string matching.
No vim language binding dependencies, use job-start API to avoid blocking vim

ToDo
----
 - Reduce hard coding
 - Remove rg executables dependencies
 - Add backtrace
 - Add tests
 - Improve fuzzy algorithm
 - Allow custom keybinding
 - UTF-8 support
 - Test on Windows

Runtime requirements
--------------------
 - Vim with vim9 script support
 - Rg (Only for the default list command)

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
    var output = system("git rev-parse --show-toplevel")
    if v:shell_error == 0
        return "cd " .. root_dir .. " && git ls-files | egrep -v '^.*(\.png|\.jpg)$' && git clean --dry-run -d | awk '{print $3}'"
    endif
    return "find " .. root_dir .. " -type f -maxdepth 2"
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
