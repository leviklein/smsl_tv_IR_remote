
sudo mkdir -p /etc/tv_smsl
sudo cp smsl_ir_codes /etc/tv_smsl

sudo cp irrp.py tv_smsl.py /usr/local/bin
sudo chmod u+x /usr/local/bin/irrp.py

# sudo cp tv_smsl.service /etc/systemd/system

# sudo systemctl daemon-reload
# sudo systemctl start tv_smsl.service
# sudo systemctl enable tv_smsl.service
# sudo systemctl restart tv_smsl.service

npm install

echo 
echo "Done configuring!"