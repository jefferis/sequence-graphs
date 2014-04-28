#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "kseq.h"
#include "util.hpp"

#include "FMDIndexBuilder.hpp"

/**
 * Utility function to report last error and kill the program.
 */
void report_error(const std::string message) {

    // Something went wrong. We need to report this error.
    int errorNumber = errno;
    char* error = strerror(errno);
    
    // Complain about the error
    std::cerr << message << " (" << errorNumber << "): " <<
        error << std::endl;
    
    // Don't finish our program.
    throw std::runtime_error(message);
    
}

// Tell kseq what files are (handle numbers) and that you read them with read.
// Don't hook in .gz support. See <http://stackoverflow.com/a/19390915/402891>
KSEQ_INIT(int, read)

FMDIndexBuilder::FMDIndexBuilder(const std::string& basename):
    basename(basename) {
    // Nothing to do. Already initialized our basename.    
}

void FMDIndexBuilder::add(const std::string& filename) {
    
    // Make a new temporary directory with our utility function.
    std::string tempDir = make_tempdir();
    
    // Work out the name of the basename file that will store the haplotypes
    std::string haplotypeFilename = tempDir + "/haplotypes";
    
    // Open it up for writing
    std::ofstream haplotypeStream;
    haplotypeStream.open(haplotypeFilename.c_str(), std::ofstream::out);
    
    // Open the main index contig size list for appending
    std::ofstream contigStream;
    contigStream.open((basename + ".chrom.sizes").c_str(), std::ofstream::out |
        std::ofstream::app);
        
    std::cout << "Writing contig data to " << (basename + ".chrom.sizes").c_str() << std::endl;
        
    // Open the FASTA for reading.
    FILE* fasta = fopen(filename.c_str(), "r");
    
    if(fasta == NULL) {
        report_error("Failed to open FASTA " + filename);
    }
    
    int fileNumber = fileno(fasta);
    
    kseq_t* seq = kseq_init(fileNumber); // Start up the parser
    while (kseq_read(seq) >= 0) { // Read sequences until we run out.
        // Stringify the sequence name
        std::string name(seq->name.s);
        
        // And the sequence sequence
        std::string sequence(seq->seq.s);
        
        // Upper-case all the letters. TODO: complain now if any not-base
        // characters are in the string.
        boost::to_upper(sequence);
        
        // Write the sequence forwards to the haplotypes file, terminated by
        // null.
        haplotypeStream << sequence << '\0';
        
        // Write the sequence in reverse complement to the haplotype file,
        // terminated by null.
        haplotypeStream << reverse_complement(sequence) << '\0';
        
        // Write the sequence ID and size to the contig list.
        contigStream << name << '\t' << sequence.size() << std::endl;
    }  
    kseq_destroy(seq); // Close down the parser.
    
    // Close up streams
    haplotypeStream.flush();
    haplotypeStream.close();
    contigStream.flush();
    contigStream.close();

    // Configure build_rlcsa by writing a configuration file.
    std::ofstream configStream;
    configStream.open((tempDir + "/haplotypes.rlcsa.parameters").c_str(),
        std::ofstream::out);
        
    configStream << "RLCSA_BLOCK_SIZE = 32" << std::endl;
    configStream << "SAMPLE_RATE = 1" << std::endl;
    configStream << "SUPPORT_DISPLAY = 1" << std::endl;
    configStream << "SUPPORT_LOCATE = 1" << std::endl;
    configStream << "WEIGHTED_SAMPLES = 0" << std::endl;
    
    configStream.flush();
    configStream.close();

    // Index the haplotypes file with build_rlcsa. Use some hardcoded number
    // of threads.
    int pid = fork();
    if(pid == 0) {
        // We're the child; execute the process.
        
        // What file does it need to index?
        const char* toIndex = haplotypeFilename.c_str();
        errno = 0;
        // Make sure to fill in its argv[0], and end with a NULL.
        if(execlp("build_rlcsa", "build_rlcsa", toIndex, "10", NULL) == -1) {
            report_error("Failed to exec build_rlcsa");
        }
    } else {
        // Wait for the child to finish.
        // This will hold its status
        int status = 0;
        waitpid(pid, &status, 0);
        if(status != 0) {
            // Complain about the error
            std::cout << "The indexing child process failed with code " << 
                status << std::endl << std::endl;
            std::cout << "Arguments were: " << "build_rlcsa" << " " <<
                haplotypeFilename << " " << "10" << std::endl;
            throw std::runtime_error("The indexing child process failed.");
        }
    }
    
    // Now merge in the index
    merge(haplotypeFilename);
    
    // Get rid of the temporary index files
    boost::filesystem::remove_all(tempDir);

}

void FMDIndexBuilder::merge(const std::string& otherBasename) {
    if(boost::filesystem::exists(basename + ".rlcsa.array")) {
        // We have an index already. Run a merge command.
        int pid = fork();
        if(pid == 0) {
            // We're the child; execute the process. Make sure to fill in its
            // argv[0], and to terminate with a NULL argument.
            execlp("merge_rlcsa", "merge_rlcsa", basename.c_str(),
                otherBasename.c_str(), "10", NULL);
        } else {
            // Wait for the child to finish.
            waitpid(pid, NULL, 0);
        }
    } else {
        // Take this index, renaming it to basename.whatever
        boost::filesystem::copy_file(otherBasename + ".rlcsa.array",
            basename + ".rlcsa.array");
        boost::filesystem::copy_file(otherBasename + ".rlcsa.parameters",
            basename + ".rlcsa.parameters");
        boost::filesystem::copy_file(otherBasename + ".rlcsa.sa_samples",
            basename + ".rlcsa.sa_samples");
    }

}



















