#!/usr/bin/env python2.7
"""
sam2tsv.py: Take a SAM file and output a TSV of
<reference>\t<position>\t<query>\t<position>\t<isBackwards> lines for each
mapped base.

"""

import argparse, sys, os, os.path, random, subprocess, shutil, itertools

from Bio import SeqIO
import pysam
import tsv

def parse_args(args):
    """
    Takes in the command-line arguments list (args), and returns a nice argparse
    result with fields for all the options.
    
    Borrows heavily from the argparse documentation examples:
    <http://docs.python.org/library/argparse.html>
    """
    
    # Construct the parser (which is stored in parser)
    # Module docstring lives in __doc__
    # See http://python-forum.com/pythonforum/viewtopic.php?f=3&t=36847
    # And a formatter class so our examples in the docstring look good. Isn't it
    # convenient how we already wrapped it to 80 characters?
    # See http://docs.python.org/library/argparse.html#formatter-class
    parser = argparse.ArgumentParser(description=__doc__, 
        formatter_class=argparse.RawDescriptionHelpFormatter)
    
    # General options
    parser.add_argument("--samIn", required=True,
        default=sys.stdin,
        help="SAM file to convert")
    parser.add_argument("--reference", type=argparse.FileType("r"), 
        required=True,
        default=sys.stdin,
        help="reference FASTA file")
    parser.add_argument("--tsvOut", type=argparse.FileType("w"),
        default=sys.stdout,
        help="TSV of mappings to write")
    
    
    # The command line arguments start with the program name, which we don't
    # want to treat as an argument for argparse. So we remove it.
    args = args[1:]
        
    return parser.parse_args(args)
    
                
def main(args):
    """
    Parses command line arguments and do the work of the program.
    "args" specifies the program arguments, with args[0] being the executable
    name. The return value should be used as the program's exit code.
    """
    
    options = parse_args(args) # This holds the nicely-parsed options object
    
    # Open the SAM file to read from.
    sam = pysam.Samfile(options.samIn, "r")
    
    # And the output TSV to write
    writer = tsv.TsvWriter(options.tsvOut)
    
    # Read the reference, which we need to be able to recognize mismatches
    # without parsing the somewhat incomprehensible MD tag.
    reference = SeqIO.to_dict(SeqIO.parse(options.reference, "fasta"))
    
    for read in sam.fetch():
        # For each read
        
        for query_pos, ref_pos in read.aligned_pairs:
            # We can just iterate through this super simply.
            
            if ref_pos is None or query_pos is None:
                # Supposedly unaligned things can sneak in here? The pysam docs
                # say so, so we skip them.
                continue
                
            # Look up the reference sequence name
            ref_name = sam.getrname(read.tid)
            
            if read.seq[query_pos] == reference[ref_name][ref_pos]:
                # This is a match, and not a mismatch.
                # Write a line for it.
                writer.line(ref_name, ref_pos, read.qname, query_pos, 
                    1 if read.is_reverse else 0)

if __name__ == "__main__" :
    sys.exit(main(sys.argv))