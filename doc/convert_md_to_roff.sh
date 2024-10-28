#!/bin/bash

PROJECT="ptrscan"
VERSION="v1.0.2"
DATE="Oct 2024"

SRC_DIR="md"
DST_DIR="roff/man1"

#check that pandoc is installed
if ! command -v pandoc &> /dev/null; then
    echo "[error]: Pandoc is not installed." >&2
    exit 1
fi

#create the dst dir if it doesn't exist
mkdir -p ${DST_DIR}

#clean previous conversions
rm ${DST_DIR}/*

#convert all markdown files to roff
for file in "$SRC_DIR"/*.md; do
    if [ -f "$file" ]; then
        filename=$(basename "$file" .md)
        dst_file="$DST_DIR/$filename.1"

        echo "Converting $file to $dst_file..."
        pandoc -s -t man "$file" -o "$dst_file"
        if [ $? -ne 0 ]; then
            echo "[error]: Pandoc failed to convert $file to roff format." >&2
        fi
    fi
done

#remove incorrect formatting
for file in "$DST_DIR"/*; do
    if [[ -f "$file" ]]; then
        sed -i '/.TH "" "" "" ""/d' "$file"
    fi
done

#add title and name to each file
for file in "$DST_DIR"/*; do

  basefile=$(basename "$file")
  namefile=${basefile%.*}
  filename=$(echo $namefile | tr [A-Z] [a-z])
  FILENAME=$(echo $namefile | tr [a-z] [A-Z])

  # add the filename to the beginning of the file in all uppercase
  echo ".TH ${FILENAME} 1 \"${DATE}\" \"${PROJECT} ${VERSION}\" \"${filename}\"" | cat - $file > temp && mv temp $file

  # add the filename to the beginning of the file in all lowercase
  echo ".IX Title \"${FILENAME} 1"  | cat - $file > temp && mv temp $file

  new_filename="${PROJECT}_$basefile"
  mv "$DST_DIR"/"$basefile" "$DST_DIR"/"$new_filename"

done

echo "Conversion complete."
