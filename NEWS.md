# RevBayes 1.2.2 (Jun 7, 2023)

## Features
  * We now use a flag (-j) instead of a separate executable for Jupyter mode.
  * Matrices can be added, subtracted, and negated.
  * Allow computing SSE likelihoods using TensorPhylo
  * Add pipe operator: `x |> f(y)` now yields `f(x,y)`.
  * ... and many bug fixes.

# RevBayes 1.2.1 "Peitenimi" (Nov 7, 2022)

## Speed & memory
  * Cache transition probability matrices.
  * 4x faster tree summaries.
  * Discrete character data sets take 5x less RAM, and are 5x faster to load.

## Features
  * Add Occurence Birth-Death Process.
  * Better mixing statistics for MC^3.

  * Automatically remove degree-2 root nodes when reading non-clock trees.

## Bug Fixes
  * Fix some problems with the BDSTP.
  * Fix false claim of conflicting constraints. (#250, #288)
  * Don't get stuck on MCMC with amino-acid models.
  * Fix crash in unrooted NNI and SPR.
  * Fix using file paths and directories on Windows.
  * Restore dnFBDP synonym for dnBDSTP.
  * Restore initialTree argument to dnFBDP .
  * Fix check for number of rates in FBD-Range process.
  * Fix crash when reading some trees with sampled ancestors. (#240)
  * Prevent NumStates methods from overriding character exclusion. (#252)
  * C++ standard switched to C++17.

# RevBayes 1.2.0 "Lagerstätte" (Aug 3, 2022)

## New MCMC moves

 * slice sampling (mvSlice) can now do geometric scaling.

## New models/analyses

 * New codon models:
   * Goldman-Yang (1994), Muse-Gaut (1994), FMutSel (Yang 2008)
   * Create of Codon models using stackable blocks fndNdS, fnX3, fnMutSel, fnMutSelAA
   * i.e. fnMutSel( fndNdS( fnX3( fnGTR(er, pi), omega ), F ) # GTR + X3 + dNdS + MutSel
 * New dinucleotide models: fnX2, fnMutSel
   * i.e. fnMutSel( fnX2( fnHKY(kappa, nuc_pi) ), F )    # HKY + X2 + MutSel
 * New birth death models:
   * Birth death sampling treatment process (Magee et al. 2020)
   * Time-heterogeneous fossilized birth death range process
  
## New features

  * optional weighting in fnAverageDistanceMatrix
  * FBD range model uses individual fossil occurrence data
  * site mixture allocations work with codon models
  * automatic handling of whitespace delimited files

## Bug fixes

  * fixed occasional crashes when using recovering a tree from a checkpoint file due to rounding. Checkpointing now records doubles without rounding
  * fixed a bug when printing individual elements of average distance matrices and calculating their completeness


# RevBayes 1.1.1

## Changes

 * **Warning**: this version includes changes to the Random Number Generator, meaning analysis output will be different from v1.1.0, even when run with the same seed.

## New models/analyses

 * new PoMo rate matrices with selection

## Bug fixes

 * fixed an error in the likelihood calculation for discrete characters when using ambiguous characters and partitioning standard data by state number
 * fixed a bug causing unbounded likelihoods when using PhyloBrownianProcessREML with unrooted trees
 * fixed a crash when using ordered rate matrices
 * fixed a crash when printing the value of a rate matrix
 * fixed a crash when using nested clade constraints
 * fixed a crash when using DEC models
 * fixed an issue with using simplex indexes in deterministic nodes
 * fixed a bug allowing NodeTimeSlideUniform to move sampled ancestor nodes
