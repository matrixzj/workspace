#! /bin/bash

## Tmux Plugin Manager Installation
[ -d ~/.tmux ] || mkdir -p ~/.tmux/plugins
git clone https://github.com/tmux-plugins/tpm ~/.tmux/plugins/tpm

# tmux powerline theme setup
[ -d ~/downloads/tmux-powerline ] && rm -rf  ~/downloads/tmux-powerline
os=$(uname)

[ -d ~/downloads ] || mkdir -p ~/downloads
git clone https://github.com/erikw/tmux-powerline.git ~/downloads/tmux-powerline

sed -i.bak '/TMUX_POWERLINE_PATCHED_FONT_IN_USE_DEFAULT/s/true/false/' ~/downloads/tmux-powerline/config/defaults.sh
# Location: Abu Dhabi 
sed -i.bak '/TMUX_POWERLINE_SEG_WEATHER_LOCATION/a\TMUX_POWERLINE_SEG_WEATHER_LAT="24.47"' ~/downloads/tmux-powerline/segments/weather.sh
sed -i.bak '/TMUX_POWERLINE_SEG_WEATHER_LOCATION/a\TMUX_POWERLINE_SEG_WEATHER_LON="54.37"' ~/downloads/tmux-powerline/segments/weather.sh
sed -i.bak '/wan_ip/s/\(.*\)/#&1/;/pwd/s/\(.*\)/#&1/;/mailcount/s/\(.*\)/#&1/;/now_play/s/\(.*\)/#&1/;/battery/s/\(.*\)/#&1/;/load/s/\(.*\)/#&1/' ~/downloads/tmux-powerline/themes/default.sh

if [[ ${os} = "Darwin" ]]; then
    ln -sf ~/Documents/workspace/tmux/tmux.conf.mac ~/.tmux.conf
else
    ln -sf ~/Documents/workspace/tmux/tmux.conf ~/.tmux.conf
fi

# bashrc setup
cat << EOF >> ~/.bashrc
alias ta='tmux attach'
alias ll='ls -al --color=auto'
# alias vim='/usr/local/bin/vim'
alias pbcopy='xsel --clipboard --input'
alias pbpaste='xsel --clipboard --output'

# BEGIN ANSIBLE MANAGED BLOCK
PS1='\n\
\[\e[0;33m\]\u@\H\[\e[0;30;1m\]:\[\e[0;33m\]\w \[\e[0;31;1m\]· \@ \d ·\[\e[0m\] \n\
\[\e[0;31m\]!\! \[\e[0;37;1m\]\$\[\e[0m\] '
# END ANSIBLE MANAGED BLOCK

export HISTTIMEFORMAT="%d/%m/%y %T "
export HISTFILESIZE=10000
EOF

source ~/.bashrc

# vim setup
mkdir -p ~/.vim/colors
ln -sf $(pwd)/vim/oblivion.vim ~/.vim/colors/oblivion.vim
ln -sf $(pwd)/vim/oblivion.vim ~/.vim/colors/Oblivion.vim
ln -sf $(pwd)/vim/vimrc ~/.vimrc

# zsh setup
# Install: sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
[ -d ~/.oh-my-zsh/themes/ ] || mkdir -p ~/.oh-my-zsh/themes/
cp $(pwd)/matrix.zsh-theme ~/.oh-my-zsh/themes/
sed -i '/^ZSH_THEME/s/=.*/="matrix"/' ~/.zshrc
echo 'export LC_ALL=en_US.UTF-8' >> ~/.zshrc
echo 'export LANG=en_US.UTF-8' >> ~/.zshrc
