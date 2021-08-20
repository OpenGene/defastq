# defastq
Ultra-fast Multi-threaded FASTQ Demultiplexing

# get defastq
## download binary 
This binary is only for Linux systems: http://opengene.org/defastq/defastq
```shell
# this binary was compiled on CentOS, and tested on CentOS/Ubuntu
wget http://opengene.org/defastq/defastq
chmod a+x ./defastq
```
## or compile from source (depend on libdeflate and libisal)
### Step 1: download and build libisal
See https://github.com/spdk/isa-l
### step 2: download and build libdeflate
See https://github.com/ebiggers/libdeflate
### step 3: download and build defastq
Please make sure that the fold `isa-l`, `libdeflate` and `defastq` are in the same directory.
```shell
# get source (you can also use browser to download from master or releases)
git clone https://github.com/OpenGene/defastq.git

# build
cd defastq
make

# Install
sudo make install
```

# all options
```
usage: ./defastq --in1=string --barcode_place=string --index=string [options] ... 
options:
  -1, --in1                   input file name for read1 (string)
  -2, --in2                   input file name for read2 (string [=])
  -b, --barcode_place         For MGI it should be read1 or read2, for Illumina, it should be index1/index2/both_index (string)
  -s, --barcode_start         If barcode_place is read1 or read2, the barcode starting position should be specified. This is 1-based. (int [=0])
  -l, --barcode_length        If barcode_place is read1 or read2, the barcode length should be specified (int [=0])
  -i, --index                 a CSV/TSV/FASTA file contains two values (filename, barcode) (string)
  -r, --reverse_complement    specify this if the index barcodes are reverse complement.
  -o, --out_folder            output folder, default is current working directory (string [=.])
  -u, --undecoded             the file name to store undetermined reads, default is 'undecoded'. To discard the undetermined reads, specify discard (string [=undecoded])
  -z, --compression           compression level for gzip output (1 ~ 12). 0 means no compression, 1 is fastest, 12 is smallest, default is 6.  (int [=6])
  -a, --allowed_mismatch      allowed mismatch (0~2) (int [=0])
  -n, --thread                number of threads (at least 4 for SE, 5 for PE), default 0 means one thread per core. (int [=0])
  -m, --memory                memory limit (GB), 4GB is minimal, default 0 means unlimited. (int [=0])
      --debug                 print debug information.
  -?, --help                  print this message
```
