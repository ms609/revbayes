#ifndef RateMatrix_PoMoKN_H
#define RateMatrix_PoMoKN_H

#include "TimeReversibleRateMatrix.h"
#include <complex>
#include <vector>


namespace RevBayesCore {
    
    class EigenSystem;
    class TransitionProbabilityMatrix;
    
    /*
     */

    class RateMatrix_PoMoKN : public TimeReversibleRateMatrix {
        
    public:
        RateMatrix_PoMoKN(  long num_states, long in_k, long in_n, long in_nmr );                                                                                            //!< Construct rate matrix with n states
        RateMatrix_PoMoKN(const RateMatrix_PoMoKN& m);                                                                  //!< Copy constructor
        virtual                             ~RateMatrix_PoMoKN(void);                                                              //!< Destructor
        
        // overloaded operators
        RateMatrix_PoMoKN&             operator=(const RateMatrix_PoMoKN& r);
        
        // RateMatrix functions
        virtual RateMatrix_PoMoKN&     assign(const Assignable &m);                                                                                            //!< Assign operation that can be called on a base class instance.
        void                                                    calculateTransitionProbabilities(double startAge, double endAge, double rate, TransitionProbabilityMatrix& P) const;    //!< Calculate the transition matrix
        RateMatrix_PoMoKN*                           clone(void) const;

        void                                                    setK( long &na );
        void                                                    setN( long &ni );
        void                                                    setMu(  const std::vector<double> &m );
        void                                                    setPhi( const std::vector<double> &f );

        void                                                    update(void);
        
    private:
        void                                                    calculateCijk(void);                                                                //!< Do precalculations on eigenvectors and their inverse
        void                                                    computeOffDiagonal( void );
        void                                                    tiProbsEigens(double t, TransitionProbabilityMatrix& P) const;                      //!< Calculate transition probabilities for real case
        void                                                    tiProbsComplexEigens(double t, TransitionProbabilityMatrix& P) const;               //!< Calculate transition probabilities for complex case
        void                                                    updateEigenSystem(void);                                                            //!< Update the system of eigenvalues and eigenvectors
        
        EigenSystem*                                            eigen_system;                                                                       //!< Holds the eigen system
        std::vector<double>                                     c_ijk;                                                                              //!< Vector of precalculated product of eigenvectors and their inverse
        std::vector<std::complex<double> >                      cc_ijk;                                                                             //!< Vector of precalculated product of eigenvectors and thier inverse for complex case
        
        long                                                    K;
        long                                                    N;
        std::vector<double>                                     mu;   
        std::vector<double>                                     phi;                                                                           //!< Vector of precalculated product of eigenvectors and their inverse
                
    };
    
}

#endif


