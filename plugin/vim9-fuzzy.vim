vim9script
import autoload "../lazyload/window_handler.vim"

command! -complete=file -nargs=* Vim9FuzzyFile window_handler.StartWindow("file", <f-args>)
command! -complete=file -nargs=* Vim9FuzzyPwdFile window_handler.StartWindow("file", expand('%:p:h'))
command! -complete=file -nargs=* Vim9FuzzyMru window_handler.StartWindow("mru", <f-args>)
command! -nargs=* Vim9FuzzyPath window_handler.StartWindow("path", <f-args>)
command! -nargs=* Vim9FuzzyYank window_handler.StartWindow("yank", <f-args>)

if exists('g:vim9_fuzzy_yank_enabled') && g:vim9_fuzzy_yank_enabled
    augroup Vim9FuzzyYank
        autocmd!
        autocmd TextYankPost * window_handler.Osc52YankHist(v:event.regcontents)
    augroup END
endif

def Make(...args: list<any>): void
    const proj_root = expand('<script>:p:h') .. "/../"
    var cmd = "sh -c 'cd " .. proj_root .. " && make -j4 " .. join(args) .. "'"
    execute "!" .. cmd
enddef

command! -nargs=* Vim9FuzzyMake Make(<q-args>)
