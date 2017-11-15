scp $1 vincent@lenss-comp1.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp1.cse.tamu.edu 'bash -s' < bin/remote_make.sh &
scp $1 vincent@lenss-comp3.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp3.cse.tamu.edu 'bash -s' < bin/remote_make.sh &
scp $1 vincent@lenss-comp4.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp4.cse.tamu.edu 'bash -s' < bin/remote_make.sh &
