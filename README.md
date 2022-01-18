Vim9-fuzzy
=========

A vim plugin for finding files with approximate string matching.
No vim language binding dependencies, use job-start API to avoid blocking vim

Still work-in-progress...

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
 - Test on Windows and Mac
 - Make CI test
 - Make an initial release

Runtime requirements
--------------------
 - Vim with vim9 script support
 - Git
 - Linux
 - Rg (For non-git dir)

Usage
-----
Installation:
```sh
mkdir -p ~/.vim/pack/plugins/opt
git clone https://github.com/kohnish/vim9-fuzzy ~/.vim/pack/plugins/opt/vim9-fuzzy
cd ~/.vim/pack/plugins/opt/vim9-fuzzy
# Not available yet (See Build section)
url https://github.com/kohnish/vim9-fuzzy/releases/linux-vim9-fuzzy -o vim9-fuzzy
```
Configuration
```vim
# Enable vim9-fuzzy
packadd vim9-fuzzy
# Root path to search from (No default yet)
let git_root=system("git rev-parse --show-toplevel | tr -d '\n'")
if len(git_root) > 0
    let proj_dir = git_root
else
    let proj_dir = getcwd()
endif
let vim9_fuzzy_proj_dir=proj_dir

# Path for keeping most recently used files.(Default)
g:vim9_fuzzy_mru_path = $HOME .. "/.vim/pack/git-plugins/start/vim9-fuzzy/mru"

# Vim9-fuzzy Keymap(No default)
# Search by only file name.
nmap <C-f> :Vim9FuzzyFile<CR>
# Search by full path. (Slow with the current algorithm)
nmap <C-j> :Vim9FuzzyPath<CR>
# Search in last opened files via vim9-fuzzy.
nmap <C-l> :Vim9FuzzyMru<CR>
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
sudo dnf install libuv-devel
cd build
cmake ..
make -j`nproc`
make install/strip
```
