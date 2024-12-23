g++ -c agd_vcf.cpp -o agd_vcf.o 
g++ -static -o agd_vcf agd_vcf.o -lz -lm -lstdc++ -lc