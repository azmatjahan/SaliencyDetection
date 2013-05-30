###################################################
## FILENAME:    testTrainModel.sh
## AUTHOR:      Jimmy Lin (u5223173) 
## DATE:        2013-05-30
## DESCRIPTION: 
##     test the feature maps composed by 
##  multiscale contrast, center surround histogram,
##  Color spatial distribution
##
###################################################
## Edited by MacVim
## Documentation autogenerated by snippet
###################################################

## six argument for this program
##  const char *imgDir = DRWN_CMDLINE_ARGV[0]; // directory restores original images
##  const char *mscDir = DRWN_CMDLINE_ARGV[1]; // directory restores multiscale contrast feature map
##  const char *cshDir = DRWN_CMDLINE_ARGV[2]; // directory restores center surround histogram feature map
##  const char *csdDir = DRWN_CMDLINE_ARGV[3]; // directory restores color spatial distribution feature map
##  const char *lblFile = DRWN_CMDLINE_ARGV[4]; // a single text file with ground truth rectangle
##  const char *outputDir = DRWN_CMDLINE_ARGV[5]; // directory for resulting images

## 

cp=$(pwd)

resultDir="../results"
mscDir="$resultDir/BMultiscaleContrast"
cshDir="$resultDir/BCenterSurround"
csdDir="$resultDir/BColorSpatialDistribution"
outdir="../results/BComposed"
index=$1 ## index of image package

cd $outdir && mkdir $index
cd $cp
../../../bin/trainModel ../dataset/images/B/$index/ $mscDir/$index/ $cshDir/$index/ $csdDir/$index/ ../dataset/labels/B/"$index"_data.txt $outdir/$index/

## run -x ../dataset/images/B/5/ ../results/BMultiscaleContrast/5/ ../results/BCenterSurround/5/ ../results/BColorSpatialDistribution/5/ ../dataset/labels/B/5_data.txt ../results/BComposed/5/
