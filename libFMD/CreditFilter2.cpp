#include "CreditFilter2.hpp"
#include <algorithm>
#include <cmath>
#include "Log.hpp"

CreditFilter2::CreditFilter2(const FMDIndex& index, 
    const GenericBitVector& ranges, size_t z_max): index(index), ranges(ranges),
    z_max(z_max), disambiguate(index) {
    // Nothing to do!
}

std::vector<Mapping> CreditFilter2::apply(
    const std::vector<Mapping>& leftMappings,
    const std::vector<Mapping>& rightMappings, const std::string& query) {

    // First disambiguate everything. This also makes combined left and right
    // contexts.
    std::vector<Mapping> disambiguated = disambiguate.apply(leftMappings,
        rightMappings);

    // Find the left and right sentinels (must be mapeped on the left/right
    // respectively, and not have the other side disagree). Maybe this can be
    // solved by disambiguation.
    
    int64_t leftSentinel = -1;
    int64_t rightSentinel = -1;
    
    for(size_t i = 0; i < disambiguated.size(); i++) {
        // Look in from the left for the left sentinel
        if(leftMappings[i].isMapped() && 
            disambiguated[i].isMapped()) {
            
            // This is a left-mapped thing on the left, and may be a sentinel.
            // To be sure, we have to find its word.
            
            // Get the length of its word, which has its right end at i
            size_t wordLength = disambiguated[i].getLeftMinContext();
            
            // Clip out the word that the base mapped on on the left.
            std::string word = query.substr(i + 1 - wordLength, wordLength);
            
            if(index.misMatchCount(ranges, word, z_max).is_mapped) {
                // This word appears exactly once within the specified number of
                // mismatches. So this is the leftmost left sentinel.
                leftSentinel = i;
                Log::info() << "Left sentinel found at " << leftSentinel << 
                    std::endl;
                break;
            }
        }
    }
    
    for(size_t i = disambiguated.size() - 1; i != (size_t) -1; i--) {
        // Look in from the right for the right sentinel
        if(rightMappings[i].isMapped() && 
            disambiguated[i].isMapped()) {
            
            // This is a right-mapped thing on the right, and may be a sentinel.
            // To be sure, we have to find its word.
            
            // Get the length of its word, which has its left end at i
            size_t wordLength = disambiguated[i].getRightMinContext();
            
            // Clip out the word that the base mapped on on the right.
            std::string word = query.substr(i, wordLength);
            
            if(index.misMatchCount(ranges, word, z_max).is_mapped) {
                // This word appears exactly once within the specified number of
                // mismatches. So this is the rightmost right sentinel.
                rightSentinel = i;
                Log::info() << "Right sentinel found at " << rightSentinel << 
                    std::endl;
                break;
            }
        }
    }
    
    if(leftSentinel == -1 || rightSentinel == -1 || 
        rightSentinel <= leftSentinel) {
        
        // We didn't find one of the sentinels, or there is no space between
        // them. We can't do credit between them. Just disambiguate without
        // applying credit.
        Log::info() << "No sequence between sentinels. No credit applied." <<
            std::endl;
        return std::move(disambiguated);
    }
    
    // Find the max left and right contexts we need to worry about checking
    // consistency from.
    size_t maxLeftContext = 0;
    size_t maxRightContext = 0;
    
    for(size_t i = 0; i < disambiguated.size(); i++) {
        // Take the max left and right context when we find them.
        
        Log::debug() << disambiguated[i].getLeftMaxContext() << ", " << 
            disambiguated[i].getRightMaxContext() << std::endl;
        
        maxLeftContext = std::max(maxLeftContext, 
            disambiguated[i].getLeftMaxContext());
        maxRightContext = std::max(maxRightContext, 
            disambiguated[i].getRightMaxContext());
    }
    
    Log::debug() << "Max context sizes: " << maxLeftContext << "|" << 
        maxRightContext << std::endl;
    
    // This is going to hold our output
    std::vector<Mapping> toReturn;
    
    for(size_t i = 0; i <= leftSentinel; i++) {
        // For each base before or at the left sentinel, disambiguate normally.
        toReturn.push_back(disambiguated[i]);
    }

    for(size_t i = leftSentinel + 1; i < rightSentinel; i++) {    
        // For each base between the sentinels
    
        if(disambiguated[i].isMapped()) {
            // Look at how it has been mapped
            // If it is mapped on one or more sides, disambiguate normally.
            toReturn.push_back(disambiguated[i]);   
        } else {
        
            Log::trace() << "Trying to credit map base " << i << std::endl;
        
            // Set this to true if you find a base that implies a position for
            // this one by its right context.
            bool rightFound = false;
            // This is where that base would place this one
            TextPosition rightCreditPosition(0, 0);
            // Set this if right contexts all place this base in one spot
            bool rightConsistent = true;
        
            for(size_t j = i - 1; j != (size_t) -1 && 
                (int64_t) j >= (int64_t) i - (int64_t) maxRightContext; j--) {
                // Look left from here until you find a base that maps and
                // places us.
                Log::trace() << "Checking base " << j << " on right" << 
                    std::endl;
                
                if(!rightMappings[j].isMapped() || 
                    !disambiguated[j].isMapped()) {
                    // This base never mapped, so it can't give credit.
                    continue;
                }
                
                if(disambiguated[j].getRightMaxContext() - 1 < i - j) {
                    // This base's context didn't reach all the way out, after
                    // accounting for the fact that it includes the base itself.
                    // This prevents us from mapping off the end of a contig,
                    // since these are limited by contig ends.
                    continue;
                }
                
                // OK, we imply some mapping. What is it?
                // Grab the mapped base and go forwards on the forward strand
                // (since right contexts reach forward).
                TextPosition implied = disambiguated[j].getLocation();
                // As i increases and j stays the same, we want offset to
                // become more positive.
                implied.addOffset((int64_t) i - (int64_t) j);
                
                Log::trace() << "Base " << j << " places base " << i << 
                    " at " << implied << " by right context" << std::endl;
                
                if(!rightFound) {
                    // This is the first one. Make sure all the others match.
                    rightFound = true;
                    rightCreditPosition = implied;
                } else if (rightCreditPosition != implied) {
                    // There are two or more implied locations from right
                    // contexts.
                    rightConsistent = false;
                    break;
                }
                
            }
            
            // And the same thing for left contexts
            
            // Set this to true if you find a base that implies a position for
            // this one by its left context.
            bool leftFound = false;
            // This is where that base would place this one
            TextPosition leftCreditPosition(0, 0);
            // Set this if left contexts all place this base in one spot
            bool leftConsistent = true;
        
            for(size_t j = i + 1; j < leftMappings.size() && 
                j < i + maxLeftContext; j++) {
                
                // Look right from here until you find a base that maps and
                // places us.
                
                Log::trace() << "Checking base " << j << " on left" << 
                    std::endl;
                
                if(!leftMappings[j].isMapped() || 
                    !disambiguated[j].isMapped()) {
                    // This base never mapped, so it can't give credit.
                    continue;
                }
                
                if(disambiguated[j].getLeftMaxContext() - 1 < j - i) {
                    // This base's context didn't reach all the way out, after
                    // accounting for the fact that it includes the base itself.
                    // This prevents us from mapping off the end of a contig,
                    // since these are limited by contig ends.
                    continue;
                }
                
                // OK, we imply some mapping. What is it?
                // Grab the mapped base and go backward on the forward strand
                // (since left contexts reach backward).
                TextPosition implied = disambiguated[j].getLocation();
                // As i increases and j stays the same, we want offset to
                // become less negative.
                implied.addOffset((int64_t) i - (int64_t) j);
                
                Log::trace() << "Base " << j << " places base " << i << 
                    " at " << implied << " by left context" << std::endl;
                
                if(!leftFound) {
                    // This is the first one. Make sure all the others match.
                    leftFound = true;
                    leftCreditPosition = implied;
                } else if (leftCreditPosition != implied) {
                    // There are two or more implied locations from left
                    // contexts.
                    leftConsistent = false;
                    break;
                }
                
            }
            
            // TODO: somehow call into the disambiguation filter here?
        
            if(leftFound && leftConsistent) {
                if(rightFound && rightConsistent) {
                    // We have credit from both the left and the right. Do they
                    // agree?
                    
                    if(leftCreditPosition == rightCreditPosition) {
                        // They agree! Record a successful mapping. TODO: do we
                        // want to send credit info along here as well?
                        toReturn.push_back(Mapping(leftCreditPosition));
                    } else {
                        // They disagree. Fail the mapping
                        toReturn.push_back(Mapping());
                    }
                } else {
                    // We have credit only from the left.
                    toReturn.push_back(Mapping(leftCreditPosition));
                }
            } else if(rightFound && rightConsistent) {
                // We have credit only from the right
                
                toReturn.push_back(Mapping(rightCreditPosition));
            } else {
                // We have credit from nowhere
                toReturn.push_back(Mapping());
            }
        }
        
    }
    
    for(size_t i = rightSentinel; i < disambiguated.size(); i++) {
        // Then for each base at or after the right sentinel, disambiguate
        // normally.
        toReturn.push_back(disambiguated[i]);
    }
    
    // Return the credit-mapped stuff
    return std::move(toReturn);
    
}