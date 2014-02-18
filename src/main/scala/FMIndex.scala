package edu.ucsc.genome

/**
 * A generic interface for implementations of an FMIndex and its associated
 * operations:
 *
 * Count: Find the count of all occurrences of a pattern
 * Locate: Find the position of one occurrence of the pattern
 * Extract: decompress a portion of the indexed sequence
 * 
 */
trait FMIndex {

    /**
     * Return the number of occurrences of the given pattern in the index.
     */
    def count(pattern: String): Long

    /**
     * Return the position at which the given occurrence of the given pattern
     * starts. Occurrence must be less than the count of the pattern.
     */
    def locate(pattern: String, occurrence: Long): Long
    
    /**
     * Extract and return the given range of positions.
     */
    def extract(start: Long, end: Long): String

}

/**
 * An FMIndex that supports some more useful operations.
 */
trait FancyFMIndex extends FMIndex {
    
    /**
     * Return the minimum number of characters from the iterator required to get
     * exactly one unique match, or -1 if no unique match exists.
     */
    def minUnique(characters: Iterator[String]): Long

    

}