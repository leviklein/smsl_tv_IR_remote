
scp -r /home/leviklein/repo/tv_smsl/node-backend/html/* pi@192.168.2.24:~/smsl_tv_IR_remote/node-backend/html/

# ssh pi@192.168.2.24 << EOF
#     cd ~/smsl_tv_IR_remote/
#     git pull
#     chmod +x setup.sh
#     ./setup.sh
# EOF