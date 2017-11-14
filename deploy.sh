#scp *.cc *.h *.proto Makefile vincent@lenss-comp1.cse.tamu.edu:~/vincent/
scp *.cc *.h *.proto Makefile vincent@lenss-comp1.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp1.cse.tamu.edu 'bash -s' < remote_make.sh &
scp *.cc *.h *.proto Makefile vincent@lenss-comp3.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp3.cse.tamu.edu 'bash -s' < remote_make.sh &
scp *.cc *.h *.proto Makefile vincent@lenss-comp4.cse.tamu.edu:~/vincent/
ssh vincent@lenss-comp4.cse.tamu.edu 'bash -s' < remote_make.sh &
