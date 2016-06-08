# Note: destination (-d) is the server (10.10.10.100)
# Note (09-14-2014): destination (-d) is the client (10.10.10.64)
#sudo packit -m inject -t UDP -s 10.10.10.208 -d 10.10.10.9 -S 403 -D 80 -c 100 -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'
packit -m inject -t UDP -s 10.10.10.208 -d 10.10.10.9 -S 403 -D 80 -c 300 -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'
