#include "CodonState.h"
#include "EigenSystem.h"
#include "MatrixComplex.h"
#include "MatrixReal.h"
#include "RateMatrix_revPoMoM2N.h"
#include "RbException.h"
#include "RbMathMatrix.h"
#include "TransitionProbabilityMatrix.h"

#include <cmath>
#include <string>
#include <iomanip>
#include <boost/math/special_functions/digamma.hpp>

using namespace RevBayesCore;

/** Construct rate matrix with n states */
RateMatrix_revPoMoM2N::RateMatrix_revPoMoM2N( long m ) : AbstractRateMatrix( m+1 ),
    N_virt( m ),
    mu( 2, 0.01 ),
    N_eff( 2 ),
    gen( 1.0 )

{
    
    eigen_system       = new EigenSystem(the_rate_matrix);
    c_ijk.resize(num_states * num_states * num_states);
    cc_ijk.resize(num_states * num_states * num_states);
    
    update();
}


/** Copy constructor */
RateMatrix_revPoMoM2N::RateMatrix_revPoMoM2N(const RateMatrix_revPoMoM2N& m) : AbstractRateMatrix( m ),
    N_eff( m.N_eff ),
    mu( m.mu ), 
    N_virt( m.N_virt ),
    gen( m.gen )
{
    
    eigen_system        = new EigenSystem( *m.eigen_system );
    c_ijk               = m.c_ijk;
    cc_ijk              = m.cc_ijk;
    
    eigen_system->setRateMatrixPtr(the_rate_matrix);
}


/** Destructor */
RateMatrix_revPoMoM2N::~RateMatrix_revPoMoM2N(void)
{
    
    delete eigen_system;
}


RateMatrix_revPoMoM2N& RateMatrix_revPoMoM2N::operator=(const RateMatrix_revPoMoM2N &r)
{
    
    if (this != &r)
    {
        AbstractRateMatrix::operator=( r );
        
        delete eigen_system;
        
        eigen_system        = new EigenSystem( *r.eigen_system );
        c_ijk               = r.c_ijk;
        cc_ijk              = r.cc_ijk;
        N_virt 				= r.N_virt;
        mu                  = r.mu;
        N_eff               = r.N_eff;
        gen                 = r.gen;

        eigen_system->setRateMatrixPtr(the_rate_matrix);
    }
    
    return *this;
}


double RateMatrix_revPoMoM2N::averageRate(void) const
{
    throw RbException("Missing implementation of average rate in PoMo2N.");
    
    return 1.0;
}



/** Do precalculations on eigenvectors */
void RateMatrix_revPoMoM2N::calculateCijk(void)
{
    
    if ( eigen_system->isComplex() == false )
    {
        // real case
        const MatrixReal& ev  = eigen_system->getEigenvectors();
        const MatrixReal& iev = eigen_system->getInverseEigenvectors();
        double* pc = &c_ijk[0];
        for (size_t i=0; i<num_states; i++)
        {
            for (size_t j=0; j<num_states; j++)
            {
                for (size_t k=0; k<num_states; k++)
                {
                    *(pc++) = ev[i][k] * iev[k][j];
                }
            }
        }
    }
    else
    {
        // complex case
        const MatrixComplex& cev  = eigen_system->getComplexEigenvectors();
        const MatrixComplex& ciev = eigen_system->getComplexInverseEigenvectors();
        std::complex<double>* pc = &cc_ijk[0];
        for (size_t i=0; i<num_states; i++)
        {
            for (size_t j=0; j<num_states; j++)
            {
                for (size_t k=0; k<num_states; k++)
                {
                    *(pc++) = cev[i][k] * ciev[k][j];
                }
            }
        }
    }
}


/** Calculate the transition probabilities */
void RateMatrix_revPoMoM2N::calculateTransitionProbabilities(double startAge, double endAge, double rate, TransitionProbabilityMatrix& P) const
{
    double t = rate * (startAge - endAge);
    if ( eigen_system->isComplex() == false )
    {
        tiProbsEigens(t, P);
    }
    else
    {
        tiProbsComplexEigens(t, P);
    }
}


RateMatrix_revPoMoM2N* RateMatrix_revPoMoM2N::clone( void ) const
{
    return new RateMatrix_revPoMoM2N( *this );
}




void RateMatrix_revPoMoM2N::computeOffDiagonal( void )
{
    
    MatrixReal& m = *the_rate_matrix;

    /*
     INFORMATION ABOUT PoMoTwo AND PoMoThree

     The idea of the virtual PoMos (Two and Three) resumes to mimicking a population dynamic that unfolds
     on the effective population N, using a virtual population of smaller size M.
     M is equal to 2 in PoMoTwo and 3 in PoMoThree. PoMo thee additionally includes selection.

     M defines a lighter and more efficient state-space.
     By matching the expected diversity (i.e., the proportion of fixed and polymorphic sites) in both
     the effective and the virtual populations, one can obtain scaling laws for the mutation rates and
     selection coefficients (see Borges et al. 2019 Genetics).

     These are intuitive for the M=2 case, in which the mutation rates are scaled by the
     harmonic number of N-1:
     mu'_ij = mu_ij*H_{N-1}  or equivalently rho'_ij = rho_ij*H_{N-1}  for the reverisible PoMo
     mu' and rho' correspond to the mutation rate and exchageability in a virtual population of 2 individuals

     Like the standard PoMos the virtual PoMos include both fixed and polymorphic sites:
               virtual_individuals_M   n_states   fixed_states   polymorphic_states
     PoMoTwo   2                       10         {2ai}          {1ai,1aj}
     PoMoThree 3                       16         {3ai}          {1ai,2aj} and {2ai,1aj}

     The pomo rate matrices defined here first list the fixed states {Na0}, {Na1} ...,
     these occupying positions 0:(K-1), and then polymorphic states, where K is the number of alleles

     K alleles comprise (K*K-K)/2 pairwise combinations of alleles.
     This is the number of edges in the pomo state-space. Each edge comprises M-1 polymorphic states.
     The polymorphic edges are listed in the following order a0a1, a0a2, a0a3, ..., a(K-2)aK-1
     */

    
    
    // populating the rate matrix with 0.0
    // **total waste of time with sparse matrices like pomos**
    for (int i=0; i<num_states ; i++)
    {
        for (int j=0; j<num_states; j++ )
        {
            m[i][j] = 0.0;
        }
    }


    // calculating the harmonic number of N-1
    // used to scale the mutation rates
    double harmonic_number_n = boost::math::digamma(N_eff) - boost::math::digamma(1.0);
    double harmonic_number_m = boost::math::digamma(N_virt) - boost::math::digamma(1.0);
    
    long M = N_virt;

    // this matrix renormalized by the expected divergence of the N model

    // Mutations
    //m[0][1]   = N*mu[0]*rnm*gen;    //mutation 01
    //m[M][M-1] = N*mu[1]*rnm*gen;    //mutation 10
    m[0][2]   = N_eff*mu[0];    //mutation 01
    m[1][M]   = N_eff*mu[1];    //mutation 10
    
    // diagonal
    m[0][0] = -m[0][2];
    m[1][1] = -m[1][M];


    double cons = 1.0*harmonic_number_m/(N_eff*harmonic_number_n);
//    double cons = 1.0/(N_eff*harmonic_number_n);
    
    m[2][3]   = (M-1.0)/M*cons;  //{(N-1)a0,1a1} -> {(N-2)a0,2a1}
    m[2][0]   = (M-1.0)/M*cons;  //{(N-1)a0,1a1} -> {Na0,0a1}
    m[2][2]   = -m[2][3]-m[2][0];
    for (int v=2; v<(M-1); v++)
    {
        m[v+1][v+2] = 1.0*v*(M-v)/M*cons;   //{(N-v)a0,va1} -> {(N-v-1)a0,(v+1)a1}
        m[v+1][v]   = 1.0*v*(M-v)/M*cons;   //{(N-v)a0,va1} -> {(N-v+1)a0,(v-1)a1}
        m[v+1][v+1] = -m[v+1][v+2]-m[v+1][v];
    }
    m[M][1]   = (M-1.0)/M*cons;  //{1a0,(N-1)a1} -> {0a0,Na1}
    m[M][M-1] = (M-1.0)/M*cons;  //{1a0,(N-1)a1} -> {2a0,(N-2)a1}
    m[M][M]   = -m[M][1]-m[M][M-1];

    // set flags
    needs_update = true;

}

std::vector<double> RateMatrix_revPoMoM2N::getStationaryFrequencies( void ) const
{

//  // calculating the harmonic number of N-1
//  // used to scale the mutation rates (or exchangeabilities)
//  double harmonic_number_n = boost::math::digamma(N) - boost::math::digamma(1.0);
//  double harmonic_number_m = boost::math::digamma(M) - boost::math::digamma(1.0);
//
//  // calculating the normalization constant
//  double nc  = mu[0] + mu[1] + 2.0*mu[0]*mu[1]*N*harmonic_number_n;
//  double rnc = 1.0/nc;
//
//  // calculating the stationary vector
//  std::vector<double> stationary_freqs(M+1,0.0);
//
//  stationary_freqs[0]  = mu[1]*rnc;
//  stationary_freqs[M]  = mu[0]*rnc;
//
//  for (int v=1; v<M; v++){
//    stationary_freqs[v]  = mu[0]*mu[1]*N*M*harmonic_number_n*rnc/(v*(M-v)*harmonic_number_m);
//  }
//
//  return stationary_freqs;

    return calculateStationaryFrequencies();
}


/** Calculate the transition probabilities for the real case */
void RateMatrix_revPoMoM2N::tiProbsEigens(double t, TransitionProbabilityMatrix& P) const
{
    
    // get a reference to the eigenvalues
    const std::vector<double>& eigenValue = eigen_system->getRealEigenvalues();
    
    // precalculate the product of the eigenvalue and the branch length
    std::vector<double> eigValExp(num_states);
    for (size_t s=0; s<num_states; s++)
    {
        eigValExp[s] = exp(eigenValue[s] * t);
    }
    
    // calculate the transition probabilities
    const double* ptr = &c_ijk[0];
    double*         p = P.theMatrix;
    for (size_t i=0; i<num_states; i++)
    {
        for (size_t j=0; j<num_states; j++, ++p)
        {
            double sum = 0.0;
            for (size_t s=0; s<num_states; s++)
            {
                sum += (*ptr++) * eigValExp[s];
            }
            
            //            P[i][j] = (sum < 0.0) ? 0.0 : sum;
            (*p) = (sum < 0.0) ? 0.0 : sum;
        }
    }
}


/** Calculate the transition probabilities for the complex case */
void RateMatrix_revPoMoM2N::tiProbsComplexEigens(double t, TransitionProbabilityMatrix& P) const
{
    
    // get a reference to the eigenvalues
    const std::vector<double>& eigenValueReal = eigen_system->getRealEigenvalues();
    const std::vector<double>& eigenValueComp = eigen_system->getImagEigenvalues();
    
    // precalculate the product of the eigenvalue and the branch length
    std::vector<std::complex<double> > ceigValExp(num_states);
    for (size_t s=0; s<num_states; s++)
    {
        std::complex<double> ev = std::complex<double>(eigenValueReal[s], eigenValueComp[s]);
        ceigValExp[s] = exp(ev * t);
    }
    
    // calculate the transition probabilities
    const std::complex<double>* ptr = &cc_ijk[0];
    for (size_t i=0; i<num_states; i++)
    {
        for (size_t j=0; j<num_states; j++)
        {
            std::complex<double> sum = std::complex<double>(0.0, 0.0);
            for (size_t s=0; s<num_states; s++)
                sum += (*ptr++) * ceigValExp[s];
            P[i][j] = (sum.real() < 0.0) ? 0.0 : sum.real();
        }
    }
}


void RateMatrix_revPoMoM2N::setNEffective(double ps)
{
    N_eff = ps;
    // set flags
    needs_update = true;
}


void RateMatrix_revPoMoM2N::setMu( const std::vector<double> &r )
{
    mu = r;
    // set flags
    needs_update = true;
}

void RateMatrix_revPoMoM2N::setGen(double g)
{
    gen = g;
    // set flags
    needs_update = true;
}


/** Update the eigen system */
void RateMatrix_revPoMoM2N::updateEigenSystem(void)
{
    
    eigen_system->update();
    calculateCijk();
    
}


void RateMatrix_revPoMoM2N::update( void )
{
    
    if ( needs_update )
    {
        // compute the off-diagonal values
        computeOffDiagonal();
        
        // set the diagonal values
//        setDiagonal();
        
        // rescale
        //rescaleToAverageRate(1.0);
        
        // now update the eigensystem
        updateEigenSystem();
        
        // clean flags
        needs_update = false;
    }
}



