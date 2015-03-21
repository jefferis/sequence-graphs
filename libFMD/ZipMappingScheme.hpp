#ifndef ZIPMAPPINGSCHEME_HPP
#define ZIPMAPPINGSCHEME_HPP

#include "MappingScheme.hpp"
#include "Mapping.hpp"
#include "IntervalIndex.hpp"
#include "Log.hpp"
#include "Matching.hpp"

#include <iomanip>

#include <unordered_map>

/**
 * Mapping scheme supporting mapping to graphs, where you zip together maximal
 * matches looking out in both directions from each position. Finds a subset of
 * the unique substrings for each base that the natural mapping scheme finds,
 * but will never map a base that the actual natural to-graph mapping scheme
 * would call as conflicted.
 */
class ZipMappingScheme: public MappingScheme {

public:
    /**
     * Inherit the constructor.
     */
    using MappingScheme::MappingScheme;
    
    /**
     * Map the given query string according to the natural mapping scheme with
     * optional inexact credit. When a mapping is found, the callback function
     * will be called with the query base index, and the TextPosition to which
     * it maps in the forward direction.
     *
     */
    virtual void map(const std::string& query,
        std::function<void(size_t, TextPosition)> callback) const override;
        
        
    // Mapping scheme parameters
    
    /**
     * Determines whether the retraction dynamic programming code will be used
     * to try and find shorter left and right contexts that are unique taken
     * together.
     */
    bool useRetraction = true;
    
    /**
     * What is the minimum total context length to accept?
     */
    size_t minContextLength = 20;
    
    /**
     * What is the maximum number of BWT merged ranges we will check for a place
     * where left and right contexts will agree on, at each retraction step.
     */
    size_t maxRangeCount = 10;
    
    /**
     * What is the maximum number of bases that we are willing to extend through
     * when trying to confirm if a context unique on one side is consistent with
     * the other?
     * TODO: Make this work for more than just the very top level.
     */
    size_t maxExtendThrough = 20;
    
protected:

    /**
     * Use the inchworm algorithm to find the longest right context present in
     * the reference for each base in the query. Results are in the same order
     * as the characters in the string, and consist of an FMDPosition of search
     * results and a context length.
     */
    std::vector<std::pair<FMDPosition, size_t>> findRightContexts(
        const std::string& query) const;
    
    /**
     * Explore all retractions of the two FMDPositions. Return either an empty
     * set (if no explored retraction finds overlapping positions between the
     * two sides), a set with one element (if we find exactly one such
     * overlap), or a set with two (or more) elements (if we find multiple
     * overlaps).
     */
    std::set<TextPosition> exploreRetractions(const FMDPosition& left,
        size_t patternLengthLeft, const FMDPosition& right,
        size_t patternLengthRight) const;    
    
};

#endif
