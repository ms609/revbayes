#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <vector>

#include "AbstractFossilizedBirthDeathProcess.h"
#include "DagNode.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RbConstants.h"
#include "RbMathCombinatorialFunctions.h"
#include "RbMathLogic.h"
#include "RbException.h"
#include "RbMathFunctions.h"
#include "RbVector.h"
#include "RbVectorImpl.h"
#include "Taxon.h"
#include "TimeInterval.h"
#include "TypedDagNode.h"

using namespace RevBayesCore;

/**
 * Constructor. 
 * We delegate most parameters to the base class and initialize the members.
 *
 * \param[in]    s              Speciation rates.
 * \param[in]    e              Extinction rates.
 * \param[in]    p              Fossil sampling rates.
 * \param[in]    c              Fossil observation counts.
 * \param[in]    r              Instantaneous sampling probabilities.
 * \param[in]    t              Rate change times.
 * \param[in]    cdt            Condition of the process (none/survival/#Taxa).
 * \param[in]    tn             Taxa.
 */
AbstractFossilizedBirthDeathProcess::AbstractFossilizedBirthDeathProcess(const DagNode *inspeciation,
                                                                         const DagNode *inextinction,
                                                                         const DagNode *inpsi,
                                                                         const TypedDagNode<double> *inrho,
                                                                         const TypedDagNode< RbVector<double> > *intimes,
                                                                         const std::vector<Taxon> &intaxa,
                                                                         bool c) :
    ascending(true), homogeneous_rho(inrho), timeline( intimes ), fbd_taxa(intaxa), complete(c), origin(0.0)
{
    dirty_taxa = std::vector<bool>(fbd_taxa.size(), true);
    dirty_psi = std::vector<bool>(fbd_taxa.size(), true);
    partial_likelihood = std::vector<double>(fbd_taxa.size(), 0.0);
    stored_likelihood = std::vector<double>(fbd_taxa.size(), 0.0);

    // initialize all the pointers to NULL
    homogeneous_lambda             = NULL;
    homogeneous_mu                 = NULL;
    homogeneous_psi                = NULL;
    heterogeneous_lambda           = NULL;
    heterogeneous_mu               = NULL;
    heterogeneous_psi              = NULL;

    RbException no_timeline_err = RbException("No time intervals provided for piecewise constant fossilized birth death process");

    range_parameters.push_back( homogeneous_rho );

    heterogeneous_lambda = dynamic_cast<const TypedDagNode<RbVector<double> >*>(inspeciation);
    homogeneous_lambda = dynamic_cast<const TypedDagNode<double >*>(inspeciation);

    range_parameters.push_back( homogeneous_lambda );
    range_parameters.push_back( heterogeneous_lambda );

    if( heterogeneous_lambda != NULL )
    {
        if( timeline == NULL ) throw(no_timeline_err);

        if (heterogeneous_lambda->getValue().size() != timeline->getValue().size() + 1)
        {
            std::stringstream ss;
            ss << "Number of speciation rates (" << heterogeneous_lambda->getValue().size() << ") does not match number of time intervals (" << timeline->getValue().size() + 1 << ")";
            throw(RbException(ss.str()));
        }
    }


    heterogeneous_mu = dynamic_cast<const TypedDagNode<RbVector<double> >*>(inextinction);
    homogeneous_mu = dynamic_cast<const TypedDagNode<double >*>(inextinction);

    range_parameters.push_back( homogeneous_mu );
    range_parameters.push_back( heterogeneous_mu );

    if( heterogeneous_mu != NULL )
    {
        if( timeline == NULL ) throw(no_timeline_err);

        if (heterogeneous_mu->getValue().size() != timeline->getValue().size() + 1)
        {
            std::stringstream ss;
            ss << "Number of extinction rates (" << heterogeneous_mu->getValue().size() << ") does not match number of time intervals (" << timeline->getValue().size() + 1 << ")";
            throw(RbException(ss.str()));
        }
    }


    heterogeneous_psi = dynamic_cast<const TypedDagNode<RbVector<double> >*>(inpsi);
    homogeneous_psi = dynamic_cast<const TypedDagNode<double >*>(inpsi);

    range_parameters.push_back( homogeneous_psi );
    range_parameters.push_back( heterogeneous_psi );

    if( heterogeneous_psi != NULL )
    {
        if( timeline == NULL ) throw(no_timeline_err);

        if (heterogeneous_psi->getValue().size() != timeline->getValue().size() + 1)
        {
            std::stringstream ss;
            ss << "Number of fossil sampling rates (" << heterogeneous_psi->getValue().size() << ") does not match number of time intervals (" << timeline->getValue().size() + 1 << ")";
            throw(RbException(ss.str()));
        }
    }

    range_parameters.push_back( timeline );

    num_intervals = timeline == NULL ? 1 : timeline->getValue().size()+1;

    if ( num_intervals > 1 )
    {
        std::vector<double> times = timeline->getValue();
        std::vector<double> times_sorted_ascending = times;
        std::vector<double> times_sorted_descending = times;

        sort(times_sorted_ascending.begin(), times_sorted_ascending.end() );
        sort(times_sorted_descending.rbegin(), times_sorted_descending.rend() );

        if( times == times_sorted_descending )
        {
            ascending = false;
        }
        else if ( times != times_sorted_ascending )
        {
            throw(RbException("Interval times must be provided in order"));
        }
    }

    b_i = std::vector<double>(fbd_taxa.size(), 0.0);
    d_i = std::vector<double>(fbd_taxa.size(), 0.0);

    p_i         = std::vector<double>(num_intervals, 1.0);
    q_i         = std::vector<double>(num_intervals, 0.0);
    q_tilde_i   = std::vector<double>(num_intervals, 0.0);

    birth       = std::vector<double>(num_intervals, 0.0);
    death       = std::vector<double>(num_intervals, 0.0);
    fossil      = std::vector<double>(num_intervals, 0.0);
    times       = std::vector<double>(num_intervals, 0.0);

    updateIntervals();

    analytic    = std::vector<bool>(fbd_taxa.size(), true);
    o_i         = std::vector<double>(fbd_taxa.size(), 0.0);
    y_i         = std::vector<size_t>(fbd_taxa.size(), 0.0);
    x_i         = std::vector<std::vector<double> >(fbd_taxa.size(), std::vector<double>() );
    nu_j        = std::vector<std::vector<double> >(fbd_taxa.size(), std::vector<double>() );
    Psi_i       = std::vector<std::vector<double> >(fbd_taxa.size(), std::vector<double>() );

    for ( size_t i = 0; i < fbd_taxa.size(); i++ )
    {
        std::map<TimeInterval, size_t> ages = fbd_taxa[i].getAges();

        std::set<double> x;
        double oldest_y = 0.0;

        // get sorted unique uncertinaty break ages
        for ( std::map<TimeInterval, size_t>::iterator Fi = ages.begin(); Fi != ages.end(); Fi++ )
        {
            x.insert(Fi->first.getMin());
            x.insert(Fi->first.getMax());

            oldest_y = std::max(Fi->first.getMin(), oldest_y);
        }

        // put the sorted ages in a vector
        for ( std::set<double>::iterator it = x.begin(); it != x.end(); it++ )
        {
            x_i[i].push_back(*it);
        }

        // compute nu
        for ( size_t j = 0; j < x_i[i].size(); j++ )
        {
            size_t nu = 0;

            for ( std::map<TimeInterval, size_t>::iterator Fi = ages.begin(); Fi != ages.end(); Fi++ )
            {
                // count the number of ranges with maximum > xj
                nu += ( Fi->first.getMax() > x_i[i][j] ) * Fi->second;

                // if this observation can be the oldest occurrence
                // but its minimum age is younger than the oldest minimum
                // then we augment this taxon with an oldest occurence age
                if ( Fi->first.getMax() > oldest_y && Fi->first.getMin() < oldest_y )
                {
                    analytic[i] = false;
                }
            }

            if ( x_i[i][j] == oldest_y )
            {
                y_i.push_back(j);
            }
            nu_j[i].push_back(nu);
            Psi_i[i].push_back(0.0);
        }
    }
}

/**
 * Compute the log-transformed probability of the current value under the current parameter values.
 *
 */
double AbstractFossilizedBirthDeathProcess::computeLnProbabilityRanges( bool force )
{
    // prepare the probability computation
    updateIntervals();
    updateStartEndTimes();

    // variable declarations and initialization
    double lnProbTimes = 0.0;

    size_t num_extant_sampled = 0;
    size_t num_extant_unsampled = 0;

    // add the fossil tip age terms
    for (size_t i = 0; i < fbd_taxa.size(); ++i)
    {
        double b = b_i[i];
        double d = d_i[i];

        double o = fbd_taxa[i].getMaxAge();
        double y = fbd_taxa[i].getMinAge();

        // check model constraints
        if ( !( b > o && ((y == 0.0 && d == 0.0) || (y > 0 && y > d)) && d >= 0.0 ) )
        {
            return RbConstants::Double::neginf;
        }
        if ( d > 0.0 != fbd_taxa[i].isExtinct() )
        {
            return RbConstants::Double::neginf;
        }

        // count the number of rho-sampled tips
        num_extant_sampled   += (d == 0.0 && y == 0.0);  // l
        num_extant_unsampled += (d == 0.0 && y > 0.0); // n - m - l

        if ( dirty_taxa[i] == true || force )
        {
            partial_likelihood[i] = 0.0;

            size_t bi = l(b);
            size_t di = l(d);

            // include speciation density
            partial_likelihood[i] += log( birth[bi] );

            // multiply by q at the birth time
            partial_likelihood[i] += q(bi, b);

            // divide by q_tilde at the death time
            partial_likelihood[i] -= q( di, d, true);

            // include extinction density
            if (d > 0.0) partial_likelihood[i] += log( death[di] );

            std::map<TimeInterval, size_t> ages = fbd_taxa[i].getAges();

            if ( analytic[i] == true )
            {
                // merge sorted rate interval and age uncertainty boundaries
                std::vector<double> x; x.reserve(times.size() + x_i[i].size());
                std::vector<double>::iterator xit = std::set_union( times.begin(), times.end(), x_i[i].begin(), x_i[i].end(), x.begin() );

                size_t oi = num_intervals - 1;
                size_t nu_index = 0;

                std::vector<double> results;

                double psi_y_xj = 0.0;
                double psi_x_y = 0.0;

                std::vector<double> psi(ages.size(), 0.0);

                double q = 0.0;
                // include intermediate q terms
                for (size_t j = bi; j < oi; j++)
                {
                    q += q_i[j];
                }
                // include intermediate q_tilde terms
                for (size_t j = oi; j < di; j++)
                {
                    q += q_tilde_i[j];
                }

                double max_result = 0.0;

                // compute the integral analytically
                for ( size_t j = 0; j < x.size() - 1; j++ )
                {
                    if (x[j] > times[oi])
                    {
                        oi--;
                        q -= q_i[oi];
                        q += q_tilde_i[oi];
                    }

                    if (x[j] > x_i[i][nu_index])
                    {
                        nu_index++;
                    }

                    double delta_psi = fossil[oi]*(x[j] - x[j-1]);

                    // if we're past the minimum fossil age
                    // then update our running psi totals
                    if ( x[j] > x_i[i][0] && dirty_psi[i] == true )
                    {
                        Psi_i[i][nu_index] = 1.0;

                        // compute the product of psi in ranges whose maxima we've passed
                        for ( std::map<TimeInterval, size_t>::iterator Fi = ages.begin(); Fi != ages.end(); Fi++ )
                        {
                            size_t k = std::distance(ages.begin(), Fi);

                            if ( Fi->first.getMin() <= x[j-1] && Fi->first.getMax() >= x[j] )
                            {
                                psi[k] += delta_psi;
                            }

                            if ( Fi->first.getMax() <= x[j-1] )
                            {
                                Psi_i[i][nu_index] += log(psi[k]) * Fi->second;
                            }
                        }
                    }

                    // if we still haven't reached the oldest minimum
                    // increase the partial incomplete sampling psi
                    if ( x[j] > x_i[i][0] && x[j] <= x_i[i][y_i[i]] )
                    {
                        psi_x_y += delta_psi;
                    }
                    // skip to the oldest minimum fossil age
                    if ( x[j] < x_i[i][y_i[i]] )
                    {
                        continue;
                    }
                    // increase the oldest occurrence psi
                    psi_y_xj += delta_psi;

                    double Q = integrateQ(oi, nu_j[i][nu_index], 0,           psi_y_xj, psi_x_y)
                             - integrateQ(oi, nu_j[i][nu_index], x[j+1]-x[j], psi_y_xj, psi_x_y);

                    double res = log(Q) + Psi_i[i][nu_index] + q;

                    results.push_back(res);

                    max_result = std::max(res, max_result);
                }

                partial_likelihood[i] += RbMath::log_sum_exp(results, max_result);

                // multiply by the number of possible oldest occurrences
                partial_likelihood[i] += log(nu_j[i][y_i[i]]);
            }
            else
            {
                size_t oi = l(o_i[i]);

                // compute q terms at oldest occurrence age
                partial_likelihood[i] += q(oi, o_i[i], true) - q(oi, o_i[i]);

                // include intermediate q terms
                for (size_t j = bi; j < oi; j++)
                {
                    partial_likelihood[i] += q_i[j];
                }
                // include intermediate q_tilde terms
                for (size_t j = oi; j < di; j++)
                {
                    partial_likelihood[i] += q_tilde_i[j];
                }

                if ( dirty_psi[i] )
                {
                    double psi_y_o = 0.0;

                    std::vector<double> psi(ages.size(), 0.0);

                    for (size_t interval = num_intervals; interval > 0; interval--)
                    {
                        size_t j = interval - 1;

                        if ( times[j] <= fbd_taxa[i].getMinAge() )
                        {
                            continue;
                        }
                        if ( times[j+1] >= o_i[i] )
                        {
                            break;
                        }

                        // increase incomplete sampling psi
                        double dt = std::min(o_i[i], times[j]) - std::max(fbd_taxa[i].getMinAge(), times[j+1]);

                        psi_y_o += fossil[j+1]*dt;

                        // increase running psi total for each observation
                        for ( std::map<TimeInterval, size_t>::iterator Fi = ages.begin(); Fi != ages.end(); Fi++ )
                        {
                            if ( Fi->first.getMin() >= times[j] || Fi->first.getMax() < times[j+1] )
                            {
                                continue;
                            }

                            double dt = std::min(std::min(Fi->first.getMax(), o_i[i]), times[j]) - std::max(Fi->first.getMin(), times[j+1]);

                            psi[std::distance(ages.begin(), Fi)] += fossil[j+1]*dt;
                        }
                    }

                    // recompute psi term
                    Psi_i[i][0] = log(fossil[oi]);

                    double recip = 0.0;

                    // factor sum over each possible oldest observation
                    for ( std::map<TimeInterval, size_t>::iterator Fi = ages.begin(); Fi != ages.end(); Fi++ )
                    {
                        size_t k = std::distance(ages.begin(), Fi);

                        if ( Fi->first.getMax() >= o_i[i] )
                        {
                            recip += Fi->second / psi[k];
                        }

                        Psi_i[i][0] += log(psi[k]) * Fi->second;
                    }

                    // sum over each possible oldest observation
                    Psi_i[i][0] += log(recip);

                    if ( complete == false )
                    {
                        // multiply by (e^psi_y_o - 1)
                        Psi_i[i][0] += log( std::expm1(psi_y_o) );
                    }
                }

                partial_likelihood[i] += Psi_i[i][0];
            }
        }

        lnProbTimes += partial_likelihood[i];
    }

    // (the origin is not a speciation event)
    lnProbTimes -= log( birth[l(origin)] );

    // add the sampled extant tip age term
    if ( homogeneous_rho->getValue() > 0.0)
    {
        lnProbTimes += num_extant_sampled * log( homogeneous_rho->getValue() );
    }
    // add the unsampled extant tip age term
    if ( homogeneous_rho->getValue() < 1.0)
    {
        lnProbTimes += num_extant_unsampled * log( 1.0 - homogeneous_rho->getValue() );
    }

    if ( RbMath::isFinite(lnProbTimes) == false )
    {
        return RbConstants::Double::neginf;
    }

    return lnProbTimes;
}


double AbstractFossilizedBirthDeathProcess::getExtinctionRate( size_t index ) const
{

    // remove the old parameter first
    if ( homogeneous_mu != NULL )
    {
        return homogeneous_mu->getValue();
    }
    else
    {
        size_t num = heterogeneous_mu->getValue().size();

        if (index >= num)
        {
            throw(RbException("Extinction rate index out of bounds"));
        }
        return ascending ? heterogeneous_mu->getValue()[num - 1 - index] : heterogeneous_mu->getValue()[index];
    }
}


double AbstractFossilizedBirthDeathProcess::getFossilSamplingRate( size_t index ) const
{

    // remove the old parameter first
    if ( homogeneous_psi != NULL )
    {
        return homogeneous_psi->getValue();
    }
    else
    {
        size_t num = heterogeneous_psi->getValue().size();

        if (index >= num)
        {
            throw(RbException("Fossil sampling rate index out of bounds"));
        }
        return ascending ? heterogeneous_psi->getValue()[num - 1 - index] : heterogeneous_psi->getValue()[index];
    }
}


double AbstractFossilizedBirthDeathProcess::getIntervalTime( size_t index ) const
{

    if ( index == num_intervals - 1 )
    {
        return 0.0;
    }
    // remove the old parameter first
    else if ( timeline != NULL )
    {
        size_t num = timeline->getValue().size();

        if (index >= num)
        {
            throw(RbException("Interval time index out of bounds"));
        }
        return ascending ? timeline->getValue()[num - 1 - index] : timeline->getValue()[index];
    }
    else
    {
        throw(RbException("Interval time index out of bounds"));
    }
}


double AbstractFossilizedBirthDeathProcess::getSpeciationRate( size_t index ) const
{

    // remove the old parameter first
    if ( homogeneous_lambda != NULL )
    {
        return homogeneous_lambda->getValue();
    }
    else
    {
        size_t num = heterogeneous_lambda->getValue().size();

        if (index >= num)
        {
            throw(RbException("Speciation rate index out of bounds"));
        }
        return ascending ? heterogeneous_lambda->getValue()[num - 1 - index] : heterogeneous_lambda->getValue()[index];
    }
}


/**
 * \int q_tilde(t)/q(t)\Psi^{\nu-1}dPsi
 */
double AbstractFossilizedBirthDeathProcess::integrateQ(size_t i, double nu, double dt, double psi, double x) const
{
    // get the parameters
    double b = birth[i];
    double d = death[i];
    double f = fossil[i];
    double r = (i == num_intervals - 1 ? homogeneous_rho->getValue() : 0.0);

    double diff = b - d - f;
    double bp   = b*f;

    double A = sqrt( diff*diff + 4.0*bp);
    double B = ( (1.0 - 2.0*(1.0-r)*p_i[i] )*b + d + f ) / A;

    double sum = b + d + f;

    double beta_0 = 0.5*(sum + A)/f;
    double beta_1 = 0.5*(sum - A)/f;

    double w_0 = 0.5*(1-B)*std::exp(-beta_0*psi);
    double w_1 = 0.5*(1-B)*std::exp(-beta_1*psi);

    try
    {
        double tmp1 = RbMath::incompleteGamma(beta_0*(psi + f*dt), nu, false, false) / std::pow(beta_0, nu);
        double tmp2 = RbMath::incompleteGamma(beta_1*(psi + f*dt), nu, false, false) / std::pow(beta_1, nu);

        if ( complete == false )
        {
            tmp1 -= RbMath::incompleteGamma((beta_0-1)*(psi + f*dt), nu, false, false) * std::exp(x) / std::pow(beta_0-1, nu);
            tmp2 -= RbMath::incompleteGamma((beta_1-1)*(psi + f*dt), nu, false, false) * std::exp(x) / std::pow(beta_1-1, nu);
        }

        w_0 *= tmp1;
        w_1 *= tmp2;
    }
    catch(RbException&)
    {
        return RbConstants::Double::nan;
    }

    return w_0 + w_1;
}


/**
 * return the index i so that t_{i-1} > t >= t_i
 * where t_i is the instantaneous sampling time (i = 0,...,l)
 * t_0 is origin
 * t_l = 0.0
 */
size_t AbstractFossilizedBirthDeathProcess::l(double t) const
{
    return times.rend() - std::upper_bound( times.rbegin(), times.rend(), t);
}


/**
 * p_i(t)
 */
double AbstractFossilizedBirthDeathProcess::p( size_t i, double t ) const
{
    // get the parameters
    double b = birth[i];
    double d = death[i];
    double f = fossil[i];
    double r = (i == num_intervals - 1 ? homogeneous_rho->getValue() : 0.0);
    double ti = times[i];
    
    double diff = b - d - f;
    double dt   = t - ti;

    double A = sqrt( diff*diff + 4.0*b*f);
    double B = ( (1.0 - 2.0*(1.0-r)*p_i[i] )*b + d + f ) / A;

    double ln_e = -A*dt;

    double tmp = (1.0 + B) + exp(ln_e)*(1.0 - B);
    
    return (b + d + f - A * ((1.0+B)-exp(ln_e)*(1.0-B))/tmp)/(2.0*b);
}


/**
 * q_i(t)
 */
double AbstractFossilizedBirthDeathProcess::q( size_t i, double t, bool tilde ) const
{
    
    if ( t == 0.0 ) return 1.0;
    
    // get the parameters
    double b = birth[i];
    double d = death[i];
    double f = fossil[i];
    double r = (i == num_intervals - 1 ? homogeneous_rho->getValue() : 0.0);
    double ti = times[i];
    
    double diff = b - d - f;
    double dt   = t - ti;

    double A = sqrt( diff*diff + 4.0*b*f);
    double B = ( (1.0 - 2.0*(1.0-r)*p_i[i] )*b + d + f ) / A;

    double ln_e = -A*dt;

    double tmp = (1.0 + B) + exp(ln_e)*(1.0 - B);

    double q = log(4.0) + ln_e - 2.0*log(tmp);

    if (tilde) q = 0.5 * (q - (b+d+f)*dt);
    
    return q;
}


/**
 *
 *
 */
void AbstractFossilizedBirthDeathProcess::redrawOldestAge(size_t i)
{
    RandomNumberGenerator* rng = GLOBAL_RNG;

    stored_o_i = o_i;

    o_i[i] = rng->uniform01()*(fbd_taxa[i].getMaxAge() - x_i[i][y_i[i]]) + x_i[i][y_i[i]];

    dirty_psi[i]  = true;
    dirty_taxa[i] = true;
}


void AbstractFossilizedBirthDeathProcess::keepSpecialization(DagNode *toucher)
{
    dirty_psi  = std::vector<bool>(fbd_taxa.size(), false);
    dirty_taxa = std::vector<bool>(fbd_taxa.size(), false);
}


void AbstractFossilizedBirthDeathProcess::restoreSpecialization(DagNode *toucher)
{
    partial_likelihood = stored_likelihood;
    o_i = stored_o_i;

    dirty_psi  = std::vector<bool>(fbd_taxa.size(), false);
    dirty_taxa = std::vector<bool>(fbd_taxa.size(), false);
}


void AbstractFossilizedBirthDeathProcess::touchSpecialization(DagNode *toucher, bool touchAll)
{
    stored_likelihood = partial_likelihood;

    dirty_taxa = std::vector<bool>(fbd_taxa.size(), true);

    if ( toucher == timeline || toucher == homogeneous_psi || toucher == heterogeneous_psi || touchAll )
    {
        dirty_psi  = std::vector<bool>(fbd_taxa.size(), true);
    }
}


/**
 *
 *
 */
void AbstractFossilizedBirthDeathProcess::updateIntervals()
{
    for (size_t interval = num_intervals; interval > 0; interval--)
    {
        size_t i = interval - 1;

        double b = getSpeciationRate(i);
        double d = getExtinctionRate(i);
        double f = getFossilSamplingRate(i);
        double ti = getIntervalTime(i);

        birth[i] = b;
        death[i] = d;
        fossil[i] = f;
        times[i] = ti;

        if (i > 0)
        {

            double r = (i == num_intervals - 1 ? homogeneous_rho->getValue() : 0.0);
            double t = getIntervalTime(i-1);

            double diff = b - d - f;
            double dt   = t - ti;

            double A = sqrt( diff*diff + 4.0*b*f);
            double B = ( (1.0 - 2.0*(1.0-r)*p_i[i] )*b + d + f ) / A;

            double ln_e = -A*dt;

            double tmp = (1.0 + B) + exp(ln_e)*(1.0 - B);

            q_i[i-1]       = log(4.0) + ln_e - 2.0*log(tmp);
            q_tilde_i[i-1] = 0.5 * ( q_i[i-1] - (b+d+f)*dt );
            p_i[i-1]       = (b + d + f - A * ((1.0+B)-exp(ln_e)*(1.0-B))/tmp)/(2.0*b);
        }
    }
}


/**
 * Swap the parameters held by this distribution.
 * 
 * \param[in]    oldP      Pointer to the old parameter.
 * \param[in]    newP      Pointer to the new parameter.
 */
void AbstractFossilizedBirthDeathProcess::swapParameterInternal(const DagNode *oldP, const DagNode *newP)
{
    if (oldP == heterogeneous_lambda)
    {
        heterogeneous_lambda = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
    else if (oldP == heterogeneous_mu)
    {
        heterogeneous_mu = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
    else if (oldP == heterogeneous_psi)
    {
        heterogeneous_psi = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
    else if (oldP == homogeneous_lambda)
    {
        homogeneous_lambda = static_cast<const TypedDagNode<double>* >( newP );
    }
    else if (oldP == homogeneous_mu)
    {
        homogeneous_mu = static_cast<const TypedDagNode<double>* >( newP );
    }
    else if (oldP == homogeneous_psi)
    {
        homogeneous_psi = static_cast<const TypedDagNode<double>* >( newP );
    }
    else if (oldP == homogeneous_rho)
    {
        homogeneous_rho = static_cast<const TypedDagNode<double>* >( newP );
    }
    else if (oldP == timeline)
    {
        timeline = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
}
