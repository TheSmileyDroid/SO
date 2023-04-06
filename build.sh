gcc shell-2.c -o shell

# test input.txt and output.txt
./shell < input.txt > .output.txt

# compare output.txt and .output.txt
diff output.txt .output.txt