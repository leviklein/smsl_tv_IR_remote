# How I use this

```sh
ssh pi@192.168.20.230 'mkdir -p ~/tv_smsl' && scp ./* pi@192.168.20.230:~/tv_smsl && ssh pi@192.168.20.230 'cd tv_smsl && chmod a+x setup.sh && ./setup.sh' 
```