#! /bin/bash

os=$(uname)
current_shell="$0"

# Bash Config
if [[ "${current_shell}" =~ '.*bash.*' ]]; then
    grep -q 'Matrix Customized BLOCK' ~/.bashrc
    if [ $? -eq 1 ]; then
        cat << EOF >> ~/.bashrc
# BEGIN Matrix Customized BLOCK
alias ta='tmux attach'
alias ll='ls -al --color=auto'
# alias vim='/usr/local/bin/vim'
alias pbcopy='xsel --clipboard --input'
alias pbpaste='xsel --clipboard --output'

PS1='\n\
\[\e[0;33m\]\u@\H\[\e[0;30;1m\]:\[\e[0;33m\]\w \[\e[0;31;1m\]· \@ \d ·\[\e[0m\] \n\
\[\e[0;31m\]!\! \[\e[0;37;1m\]\$\[\e[0m\] '

export HISTTIMEFORMAT="%d/%m/%y %T "
export HISTFILESIZE=10000
# END Matrix Customized BLOCK
EOF
    fi
    source ~/.bashrc
fi


# oh-my-zsh Config
## Install: sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
if [[ "${current_shell}" =~ '.*zsh.*' ]]; then
    [ -d ~/.oh-my-zsh/themes/ ] || mkdir -p ~/.oh-my-zsh/themes/

    cp $(pwd)/matrix.zsh-theme ~/.oh-my-zsh/themes/
    sed -i '/^ZSH_THEME/s/=.*/="matrix"/' ~/.zshrc

    grep -q 'Matrix Customized BLOCK' ~/.zshrc
    if [ $? -eq 1 ]; then
        cat << EOF >> ~/.zshrc
# BEGIN Matrix Customized BLOCK
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
alias ll='ls -al --color=auto'
# END Matrix Customized BLOCK
EOF
    fi

    source ~/.zshrc
fi

# Tmux Config
## Tmux Plugin Manager Installation
git clone https://github.com/tmux-plugins/tpm ~/.tmux/plugins/tpm 2>/dev/null

if [[ ${os} = "Darwin" ]]; then
    ln -sf $(pwd)/tmux/tmux.conf.mac ~/.tmux.conf
elif [[ ${os} = "Linux" ]]; then
    ln -sf $(pwd)/tmux/tmux.conf.linux ~/.tmux.conf
else
    ln -sf $(pwd)/tmux/tmux.conf ~/.tmux.conf
fi

ln -sf $(pwd)/tmux/tmux-powerline/config.sh ~/.config/tmux-powerline/config.sh
[ -d ~/.config/tmux-powerline/themes ] || mkdir -p ~/.config/tmux-powerline/themes
ln -sf $(pwd)/tmux/tmux-powerline/themes/matrix.sh ~/.config/tmux-powerline/themes/matrix.sh
 
# # VIM Config
mkdir -p ~/.vim/colors
git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim 2>/dev/null
ln -sf $(pwd)/vim/oblivion.vim ~/.vim/colors/oblivion.vim
ln -sf $(pwd)/vim/oblivion.vim ~/.vim/colors/Oblivion.vim
ln -sf $(pwd)/vim/vimrc ~/.vimrc
