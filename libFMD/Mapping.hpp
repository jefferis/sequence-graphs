#ifndef MAPPING_HPP
#define MAPPING_HPP

#include <iostream>
#include <utility>
#include <stdexcept>

#include "TextPosition.hpp"

/**
 * Represents a mapping between a base in a string and a (text, index) position
 * in the FMD-index. Contains the text and offset to which a character maps, and
 * a flag to say if it represents a real mapping or a result of "unmapped". Also
 * contains how much context was used to map this position, both maximal for
 * credit and minimum unique for display.
 */
struct Mapping
{
public:
    /**
     * Make an unmapped mapping.
     */
    Mapping();
    
    /**
     * Make a no-context mapping to the given position.
     */
    Mapping(TextPosition location);
    
    /**
     * Make a mapping to the given position with equal min unique and maximal
     * contexts.
     */
    Mapping(TextPosition location, size_t leftContext, size_t rightContext);
    
    /**
     * Provide equality comparison for testing.
     */
    bool operator==(const Mapping& other) const;
    
    /**
     * Also provide inequality.
     */
    bool operator!=(const Mapping& other) const;
    
    /**
     * Set the TextPosition that this mapping is to.
     */
    inline void setLocation(TextPosition newLocation) {
        location = newLocation;
    }
    
    /**
     * What text and offset is this mapping to?
     */
    inline TextPosition getLocation() const {
        return location;
    }
    
    /**
     * Is this Mapping actually mapped?
     */
    inline bool isMapped() const {
        return is_mapped;
    }
    
    /**
     * Return the amount of context used to map on the left. This counts the
     * base itself. May be nonzero even if unmapped.
     */
    inline size_t getLeftMaxContext() const {
        return leftMaxContext; 
    }
    
    /**
     * Return the amount of context used to map on the right. This counts the
     * base itself. May be nonzero even if unmapped.
     */
    inline size_t getRightMaxContext() const {
        return rightMaxContext; 
    }
    
    /**
     * Set the max contexts available, used for credit.
     */
    inline void setMaxContext(size_t left, size_t right) {
        leftMaxContext = left;
        rightMaxContext = right;
    }
    
    /**
     * Flip this mapping and return a new mapping for the other strand, with
     * contexts swapped.
     */
    inline Mapping flip(size_t contigLength) const {
        if(is_mapped) {
            // If we're mapped, flip the location and exchange the contexts.
            TextPosition newLocation = location;
            newLocation.flip(contigLength);
            Mapping flipped(newLocation);
            flipped.setMaxContext(rightMaxContext, leftMaxContext);
            return flipped;
        } else {
            // If we are unmapped, just continue being exactly the same.
            return *this;
        }
    }
    
    // Holds (text, offset) if we are mapped to a location
    TextPosition location;
    // Holds range number if we are mapped to a range
    int64_t range;
    // Is the above actually filled in?
    bool is_mapped;
    // And how far could we go on the left?
    size_t leftMaxContext;
    // And how far could we go on the right?
    size_t rightMaxContext;
};

/**
 * Provide pretty-printing for Mappings. See
 * <http://www.parashift.com/c++-faq/output-operator.html>
 */
std::ostream& operator<< (std::ostream& o, Mapping const& mapping);

#endif
