#include "LCPArray.hpp"

// We need SAElems, so we might as well explicitly include their file.
#include <STCommon.h>

#include "Log.hpp"

#include <iterator>
#include <iostream>
#include <fstream>

LCPArray::LCPArray(const SuffixArray& suffixArray, const ReadTable& strings): values(), 
    psvs(), nsvs() {

    if(suffixArray.getSize() == 0) {
        // Just have a 0-length LCP if there are absolutely no suffixes.
        return;    
    }

    // We need to scan the suffix array and fill in the values vector.
    
    // The first value is always 0. Nothing is shared with the before-the-
    // beginning suffix. But this position (0) is also used as the "no previous
    // smaller value available" position.
    values.push_back(0);
    
    // This holds the last suffix that we need to compare the next one to.
    SAElem last = suffixArray.get(0);
    
    for(size_t i = 1; i < suffixArray.getSize(); i++) {
        // For each subsequent suffix
        
        SAElem next = suffixArray.get(i);
        
        Log::trace() << "Suffix " << next << " length " << 
            getSuffixLength(next, strings) << " vs. " << last << " length " << 
            getSuffixLength(last, strings) << std::endl;
        
        // lcp will hold the longest common prefix for this pair.
        size_t lcp;
        for(lcp = 0; lcp < getSuffixLength(last, strings) && 
            lcp < getSuffixLength(next, strings); lcp++) {
            
            // For every character that the two suffixes have
            
            Log::trace() << "Char " << getFromSuffix(next, lcp, strings) << 
                " vs. " << getFromSuffix(last, lcp, strings) << std::endl;
            
            if(getFromSuffix(last, lcp, strings) != 
                getFromSuffix(next, lcp, strings)) {
                // They don't match here, so end the longest common prefix.
                break;
            }
            
        }
        
        // Now save that prefix length
        values.push_back(lcp);
        
        // Now advance to the next suffix.
        last = next;
    }
    
    // OK, now we need to construct the PSV/NSV indexes. The easiest way is with
    // scans.
    for(size_t i = 0; i < values.size(); i++) {
        // For each suffix. TODO: iterators?
        
        // What's the position of the most recent smaller value? If none is
        // found, we'll just use 0.
        size_t psv = 0;
        
        for(size_t j = i - 1; j != (size_t) -1; j--) {
            // For each position before i in descending order (stopping on
            // underflow)...
            
            if(values[j] < values[i]) {
                // We found a smaller value.
                psv = j;
                break;
            }
        }
        
        // Stick in the PSV pointer
        psvs.push_back(psv);
    }
    
    // TODO: Just 1 scan?
    
    // Now the same for the NSV
    for(size_t i = 0; i < values.size(); i++) {
        // For each suffix
        
        // What's the position of the next smaller value? If none is found,
        // we'll just use 1 past the end.
        size_t nsv = values.size();
        
        for(size_t j = i + 1; j < values.size(); j++) {
            // For each position after i in ascending order.
            
            if(values[j] < values[i]) {
                // We found a smaller value
                nsv = j;
                break;
            }            
        }
        
        // Stick in the NSV pointer
        nsvs.push_back(nsv);
        
        
    }
    
}

LCPArray::LCPArray(const std::string& filename) : values(), psvs(), nsvs() {
    // Make a binary input stream.
    std::ifstream file(filename, std::ifstream::binary);
    
    // This is going to hold how many items should be in each vector.
    size_t arrayLength;
    
    // Read the number of items in platform-native byte order.
    file.read((char*) &arrayLength, sizeof(size_t));
    
    // Resize all the vectors
    values.resize(arrayLength);
    psvs.resize(arrayLength);
    nsvs.resize(arrayLength);
    
    // Read in that many elements for each vector
    file.read((char*) &values[0], arrayLength * sizeof(size_t));
    file.read((char*) &psvs[0], arrayLength * sizeof(size_t));
    file.read((char*) &nsvs[0], arrayLength * sizeof(size_t));
    
    // Close up the file.
    file.close();
}

void LCPArray::save(const std::string& filename) const {
    // Make a binary output stream.
    std::ofstream file(filename, std::ios::out | std::ofstream::binary);
    
    // Grab the array length as a local
    size_t arrayLength = values.size();
    
    // Save the array length in platform-native byte order.
    file.write((char*) &arrayLength, sizeof(size_t));
    
    // Read in that many elements for each vector
    file.write((char*) &values[0], arrayLength * sizeof(size_t));
    file.write((char*) &psvs[0], arrayLength * sizeof(size_t));
    file.write((char*) &nsvs[0], arrayLength * sizeof(size_t));
    
    // Close up the file
    file.close();
    
}








