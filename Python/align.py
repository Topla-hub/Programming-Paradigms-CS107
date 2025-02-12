#!/usr/bin/env python
from __future__ import print_function 
import random # for seed, random
import sys    # for stdout


################################### TEST PART ##################################
################################################################################

# Tests align strands and scores
# Parameters types:
#    score          =  int   example: -6
#    plusScores     = string example: "  1   1  1"
#    minusScores    = string example: "22 111 11 "
#    strandAligned1 = string example: "  CAAGTCGC"
#    strandAligned2 = string example: "ATCCCATTAC"
#
#   Note: all strings must have same length
def test(score, plusScores, minusScores, strandAligned1, strandAligned2):
    print("\n>>>>>>START TEST<<<<<<")

    if testStrands(score, plusScores, minusScores, strandAligned1, strandAligned2):
        sys.stdout.write(">>>>>>>Test SUCCESS:")
        sys.stdout.write("\n\t\t" + "Score: "+str(score))
        sys.stdout.write("\n\t\t+ " + plusScores)
        sys.stdout.write("\n\t\t  " + strandAligned1)
        sys.stdout.write("\n\t\t  " + strandAligned2)
        sys.stdout.write("\n\t\t- " + minusScores)
        sys.stdout.write("\n\n")
    else:
        sys.stdout.write("\t>>>>!!!Test FAILED\n\n")


# converts character score to int
def testScoreToInt(score):
    if score == ' ':
        return 0
    return int(score)


# computes sum of scores
def testSumScore(scores):
    result = 0
    for ch in scores:
        result += testScoreToInt(ch)
    return result


# test each characters and scores
def testValidateEach(ch1, ch2, plusScore, minusScore):
    if ch1 == ' ' or ch2 == ' ':
        return plusScore == 0 and minusScore == 2
    if ch1 == ch2:
        return plusScore == 1 and minusScore == 0
    return plusScore == 0 and minusScore == 1


# test and validates strands
def testStrands(score, plusScores, minusScores, strandAligned1, strandAligned2):
    if len(plusScores) != len(minusScores) or len(minusScores) != len(strandAligned1) or len(strandAligned1) != len(
            strandAligned2):
        sys.stdout.write("Length mismatch! \n")
        return False

    if len(plusScores) == 0:
        sys.stdout.write("Length is Zero! \n")
        return False

    if testSumScore(plusScores) - testSumScore(minusScores) != score:
        sys.stdout.write("Score mismatch to score strings! TEST FAILED!\n")
        return False
    for i in range(len(plusScores)):
        if not testValidateEach(strandAligned1[i], strandAligned2[i], testScoreToInt(plusScores[i]),
                                testScoreToInt(minusScores[i])):
            sys.stdout.write("Invalid scores for position " + str(i) + ":\n")
            sys.stdout.write("\t char1: " + strandAligned1[i] + " char2: " +
                             strandAligned2[i] + " +" + str(testScoreToInt(plusScores[i])) + " -" +
                             str(testScoreToInt(minusScores[i])) + "\n")
            return False

    return True

######################## END OF TEST PART ######################################
################################################################################


# Computes the score of the optimal alignment of two DNA strands.
import copy

def findOptimalAlignment(strand1, strand2, optimalAlignments):
    # Memoization: If the result is already computed, return it
    if (strand1, strand2) in optimalAlignments:
        return copy.copy(optimalAlignments[(strand1, strand2)])

    len1 = len(strand1)
    len2 = len(strand2)

    # Create a 2D matrix to store the alignment scores
    dp = [[0] * (len2 + 1) for _ in range(len1 + 1)]
    
    # Fill the first row and first column for gap penalties
    for i in range(len1 + 1):
        dp[i][0] = i * -2  # Gap penalties in strand1
    for j in range(len2 + 1):
        dp[0][j] = j * -2  # Gap penalties in strand2
    
    # Fill the matrix with optimal scores
    for i in range(1, len1 + 1):
        for j in range(1, len2 + 1):
            match = dp[i - 1][j - 1] + (1 if strand1[i - 1] == strand2[j - 1] else -1)
            gap1 = dp[i - 1][j] - 2  # Gap in strand2
            gap2 = dp[i][j - 1] - 2  # Gap in strand1
            dp[i][j] = max(match, gap1, gap2)

    # Trace back to find the optimal alignment
    aligned_strand1 = []
    aligned_strand2 = []
    i, j = len1, len2
    positiveScores = []
    negativeScores = []

    while i > 0 or j > 0:
        if i > 0 and j > 0 and dp[i][j] == dp[i - 1][j - 1] + (1 if strand1[i - 1] == strand2[j - 1] else -1):
            aligned_strand1.append(strand1[i - 1])
            aligned_strand2.append(strand2[j - 1])
            if strand1[i - 1] == strand2[j - 1]:
                positiveScores.append("1")
                negativeScores.append(" ")
            else:
                positiveScores.append(" ")
                negativeScores.append("1")
            i -= 1
            j -= 1
        elif i > 0 and dp[i][j] == dp[i - 1][j] - 2:
            aligned_strand1.append(strand1[i - 1])
            aligned_strand2.append(" ")
            positiveScores.append(" ")
            negativeScores.append("2")
            i -= 1
        else:
            aligned_strand1.append(" ")
            aligned_strand2.append(strand2[j - 1])
            positiveScores.append(" ")
            negativeScores.append("2")
            j -= 1

    # Reverse the aligned strands as we built them backward
    aligned_strand1.reverse()
    aligned_strand2.reverse()
    positiveScores.reverse()
    negativeScores.reverse()

    best = {
        "strand1": ''.join(aligned_strand1),
        "strand2": ''.join(aligned_strand2),
        "score": dp[len1][len2],
        "positiveScores": ''.join(positiveScores),
        "negativeScores": ''.join(negativeScores),
    }

    # Store the result in the memoization dictionary
    optimalAlignments[(strand1, strand2)] = best
    return copy.copy(best)




# Utility function that generates a random DNA string of
# a random length drawn from the range [minlength, maxlength]
def generateRandomDNAStrand(minlength, maxlength):
        assert minlength > 0, \
               "Minimum length passed to generateRandomDNAStrand" \
               "must be a positive number" # these \'s allow mult-line statements
        assert maxlength >= minlength, \
               "Maximum length passed to generateRandomDNAStrand must be at " \
               "as large as the specified minimum length"
        strand = ""
        length = random.choice(xrange(minlength, maxlength + 1))
        bases = ['A', 'T', 'G', 'C']
        for i in xrange(0, length):
                strand += random.choice(bases)
        return strand

# Method that just prints out the supplied alignment score.
# This is more of a placeholder for what will ultimately
# print out not only the score but the alignment as well.

def printAlignment(alignment, out = sys.stdout):
        
        out.write("\nOptimal alignment score is " + str(alignment["score"]) + "\n\n")
        out.write("  + " + str(alignment["positiveScores"]) + "\n")
        out.write("    " + str(alignment["strand1"]) + "\n")
        out.write("    " + str(alignment["strand2"]) + "\n")
        out.write("  - " + str(alignment["negativeScores"]) + "\n\n")

# Unit test main in place to do little more than
# exercise the above algorithm.  As written, it
# generates two fairly short DNA strands and
# determines the optimal alignment score.
#
# As you change the implementation of findOptimalAlignment
# to use memoization, you should change the 8s to 40s and
# the 10s to 60s and still see everything execute very
# quickly.
 
def main():
        while (True):
                sys.stdout.write("Generate random DNA strands? ")
                answer = sys.stdin.readline()
                if answer == "no\n": break
                strand1 = generateRandomDNAStrand(40, 60)
                strand2 = generateRandomDNAStrand(40, 60)
                sys.stdout.write("Aligning these two strands:\n\n")
                sys.stdout.write("   " + strand1 + "\n")
                sys.stdout.write("   " + strand2 + "\n")
                optimalAlignments = {}
                alignment = findOptimalAlignment(strand1, strand2, optimalAlignments)
                printAlignment(alignment)

                aligned_strand1 = alignment["strand1"]
                aligned_strand2 = alignment["strand2"]
                score = alignment["score"]
                plusScores = alignment["positiveScores"]
                minusScores = alignment["negativeScores"]
                
                # Ensure all strings have the same length for the test function
                max_length = max(len(aligned_strand1), len(aligned_strand2), len(plusScores), len(minusScores))
                
                # Pad shorter strings with spaces
                aligned_strand1 = aligned_strand1.ljust(max_length)
                aligned_strand2 = aligned_strand2.ljust(max_length)
                plusScores = plusScores.ljust(max_length)
                minusScores = minusScores.ljust(max_length)
                
                # Pass the result to the test function
                test(score, plusScores, minusScores, aligned_strand1, aligned_strand2)
                        
if __name__ == "__main__":
  main()