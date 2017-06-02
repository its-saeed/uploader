./server_upload -p 12345 -d /home/saeed/download/ -t 1048576 &
./client_upload -i 127.0.0.1 -p 12345 -c 5 -u /home/saeed/upload/ -t 1048576&
