# Tarball
<b>C</b> program that can <b>create</b> a Tarball and also <b>extract</b> files from it

You can find the create and extract functions in <b>mytar_routines.c</b>

## What is a tarball
A tarball is a computer software utility for collecting many files into one archive file

## Structure
It begins with the path and the size of each file and continues with the data region
![tar](https://user-images.githubusercontent.com/36489953/38105095-61a89274-338b-11e8-8a34-12ff8cab2c91.PNG)

## Usage

To run this program import the tarball folder in your eclipse C project or run it directly from the terminal

<b>Creates a tarball</b>
```
-cf tarball.mtar InputFile1 InputFile2 InputFile3 ...
```
<b>Extracts files from a tarball</b>
```
-xf tarball.mtar
```
