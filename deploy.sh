

ssh pi@192.168.20.230 << EOF
    cd ~/smsl_tv_IR_remote/
    git pull
    chmod +x setup.sh
    ./setup.sh
EOF