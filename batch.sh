#!/bin/bash
# write batch test for agd_vcf, bcftools and awk

DRAGEN_VCF="/nobackup/h_cqs/shengq2/biovu/agd163k/chrom-msvcf/chrom-25-M/postproc-vcf/dragen.vcf.gz"
ID_MAP_FILE="/nobackup/h_cqs/shengq2/biovu/AGD/SAMPLE_ID_MAP_2024_Q2_163K_primaryGRID_v5_202410.txt"

# Output file for time costs
TIME_COST_FILE="time_cost.txt"
if [[ ! -s $TIME_COST_FILE ]]; then
  echo "Time cost for each tool:" > $TIME_COST_FILE
fi

# 1. Test agd_vcf
if [[ ! -s agd_vcf.vcf.gz ]]; then
  echo "Testing agd_vcf..."
  echo "agd_vcf:" >> $TIME_COST_FILE
  { time zcat $DRAGEN_VCF | ./agd_vcf --id_map_file=$ID_MAP_FILE --total_variants=52402 | bgzip > agd_vcf.vcf.gz ; } 2>> $TIME_COST_FILE
  zcat agd_vcf.vcf.gz | grep -v "^#" | wc -l >> $TIME_COST_FILE
fi

# 2. Test bcftools
if [[ ! -s bcftools.vcf.gz ]]; then
  echo "Testing bcftools..."
  echo "bcftools:" >> $TIME_COST_FILE
  { time bcftools view -f PASS $DRAGEN_VCF -O v | bgzip > bcftools.vcf.gz ; } 2>> $TIME_COST_FILE
  zcat bcftools.vcf.gz | grep -v "^#" | wc -l >> $TIME_COST_FILE
fi

# 3. Test awk
if [[ ! -s awk.vcf.gz ]]; then
  echo "Testing awk..."
  echo "awk:" >> $TIME_COST_FILE
  { time zcat $DRAGEN_VCF | awk '$7 == "PASS" || $1 ~ /^#/' | bgzip > awk.vcf.gz ; } 2>> $TIME_COST_FILE
  zcat awk.vcf.gz | grep -v "^#" | wc -l >> $TIME_COST_FILE
fi

# test bgzip thread 4
# 1. Test agd_vcf
if [[ ! -s agd_vcf.thread4.vcf.gz ]]; then
  echo "Testing agd_vcf.thread4..."
  echo "agd_vcf.thread4:" >> $TIME_COST_FILE
  { time zcat $DRAGEN_VCF | ./agd_vcf --id_map_file=$ID_MAP_FILE --total_variants=52402 | bgzip -@ 4 -c > agd_vcf.thread4.vcf.gz ; } 2>> $TIME_COST_FILE
fi
# 2. Test awk
if [[ ! -s awk.thread4.vcf.gz ]]; then
  echo "Testing awk.thread4..."
  echo "awk.thread4:" >> $TIME_COST_FILE
  { time zcat $DRAGEN_VCF | awk '$7 == "PASS" || $1 ~ /^#/' | bgzip -@ 4 -c > awk.thread4.vcf.gz ; } 2>> $TIME_COST_FILE
fi

echo "Time cost saved to $TIME_COST_FILE"
cat $TIME_COST_FILE