Vim9-fuzzy
=========

A vim plugin for finding files with approximate string matching.
No vim language binding dependencies, use job-start API to avoid blocking vim

ToDo
----
 - Improve fuzzy algorithm
 - Colour matching characters properly
 - Reduce hard coding
 - Add backtrace
 - Add tests
 - Remove git and rg executables dependencies
 - UTF-8 support
 - Allow custom keybinding
 - Test on Windows
 - Make CI test

Runtime requirements
--------------------
 - Vim with vim9 script support
 - Git
 - Linux or Mac. (Might work on Windows)
 - Rg (For non-git dir)

Usage
-----
Installation:
```sh
mkdir -p ~/.vim/pack/plugins/opt
git clone https://github.com/kohnish/vim9-fuzzy ~/.vim/pack/plugins/opt/vim9-fuzzy
# For Linux 
curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v0.2/vim9-fuzzy-linux-x86-64 -o ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy 
chmod +x ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy
# For Windows
curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v0.2/vim9-fuzzy-win-x86-64 -o ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy.exe
chmod +x ~/.vim/pack/plugins/opt/vim9-fuzzy/bin/vim9-fuzzy.exe
# Or see the build section for compiling locally


```
Configuration
```vim
# Enable vim9-fuzzy
packadd! vim9-fuzzy

# Root path to search from (No default yet)
var proj_dir = getcwd()
var git_root = system("git rev-parse --show-toplevel | tr -d '\n'")
if len(git_root) > 0
    proj_dir = git_root
endif
g:vim9_fuzzy_proj_dir = proj_dir

# Path for keeping most recently used files.(Default)
g:vim9_fuzzy_mru_path = $HOME .. "/.vim/pack/plugins/opt/vim9-fuzzy/mru"

# Vim9-fuzzy Keymap (No defaults)
# Search by only file name.
noremap <C-f> :Vim9FuzzyFile<CR>
# Search by full path
noremap <C-y> :Vim9FuzzyPath<CR>
# Search in last opened files via vim9-fuzzy.
noremap <C-e> :Vim9FuzzyMru<CR>
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
