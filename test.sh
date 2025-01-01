#comment
cd .. #comment
cd P3
ls > out.txt 
grep world < in.txt > out.txt
ls | grep out.txt
cat in.txt | grep hello | grep world
echo test/*.txt
ls test/*.txt | grep one.txt > out.txt 
cat in.txt | grep hello | grep world | exit > exit.txt