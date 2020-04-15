#ifndef OrderedEventVectorScaleProposal_H
#define OrderedEventVectorScaleProposal_H

#include <set>
#include <string>

#include "OrderedEvents.h"
#include "Proposal.h"
#include "StochasticNode.h"

namespace RevBayesCore {
    
    template <class valueType>
    class OrderedEventVectorScaleProposal : public Proposal {
        
    public:
        OrderedEventVectorScaleProposal( StochasticNode< OrderedEvents<valueType> > *n, double l=0.5 );                                                                    //!<  constructor
        
        // Basic utility functions
        void                                cleanProposal(void);                                                                //!< Clean up proposal
        OrderedEventVectorScaleProposal*    clone(void) const;                                                                  //!< Clone object
        double                              doProposal(void);                                                                   //!< Perform proposal
        const std::string&                  getProposalName(void) const;                                                        //!< Get the name of the proposal for summary printing
        double                              getProposalTuningParameter(void) const;
        void                                prepareProposal(void);                                                              //!< Prepare the proposal
        void                                printParameterSummary(std::ostream &o, bool name_only) const;                                       //!< Print the parameter summary
        void                                setProposalTuningParameter(double tp);
        void                                tune(double r);                                                                     //!< Tune the proposal to achieve a better acceptance/rejection ratio
        void                                undoProposal(void);                                                                 //!< Reject the proposal
        
    protected:
        
        void                                swapNodeInternal(DagNode *oldN, DagNode *newN);                                     //!< Swap the DAG nodes on which the Proposal is working on
        
        
    private:

        // parameters
        StochasticNode< OrderedEvents<valueType> >*        variable;                                                                           //!< The variable the Proposal is working on
        size_t                                             lambda;
        valueType                                          old_value;
        double                                             event_time;
        bool                                               abort;
        
    };
    
}


#include "MixtureDistribution.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RbConstants.h"
#include "RbException.h"
#include "MarkovEventsDistribution.h"
#include "TypedDagNode.h"

#include <cmath>
#include <iostream>


/**
 * Constructor
 *
 * Here we simply allocate and initialize the Proposal object.
 */
template <class valueType>
RevBayesCore::OrderedEventVectorScaleProposal<valueType>::OrderedEventVectorScaleProposal( StochasticNode< OrderedEvents<valueType> > *n, double l ) : Proposal(),
    variable( n ),
    lambda( l ),
	old_value( 0.0 ),
	event_time( 0.0 ),
	abort( false )

{
    // tell the base class to add the node
    addNode( variable );
    
}


/**
 * The cleanProposal function may be called to clean up memory allocations after AbstractMove
 * decides whether to accept, reject, etc. the proposed value.
 *
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::cleanProposal( void )
{
    
}

/**
 * The clone function is a convenience function to create proper copies of inherited objected.
 * E.g. a.clone() will create a clone of the correct type even if 'a' is of derived type 'b'.
 *
 * \return A new copy of the proposal.
 */
template <class valueType>
RevBayesCore::OrderedEventVectorScaleProposal<valueType>* RevBayesCore::OrderedEventVectorScaleProposal<valueType>::clone( void ) const
{
    
    return new OrderedEventVectorScaleProposal<valueType>( *this );
}


/**
 * Get Proposals' name of object
 *
 * \return The Proposals' name.
 */
template <class valueType>
const std::string& RevBayesCore::OrderedEventVectorScaleProposal<valueType>::getProposalName( void ) const
{
    static std::string name = "OrderedEventVectorScale";
    
    return name;
}


template <class valueType>
double RevBayesCore::OrderedEventVectorScaleProposal<valueType>::getProposalTuningParameter( void ) const
{
    // this proposal has no tuning parameter
    return RbConstants::Double::nan;
}


/**
 * Perform the proposal.
 *
 * The reversible jump proposal switches the current "dimension".
 *
 * \return The hastings ratio.
 */
template <class valueType>
double RevBayesCore::OrderedEventVectorScaleProposal<valueType>::doProposal( void )
{
	// clear abort flag
	abort = false;

    // Get random number generator
    RandomNumberGenerator* rng     = GLOBAL_RNG;

    // get the distribution
    MarkovEventsDistribution<valueType>* dist = dynamic_cast<MarkovEventsDistribution<valueType> *>( &variable->getDistribution() );
    if ( dist == NULL )
    {
    	throw RbException("Tried to use OrderedEventVectorScaleProposal on an invalid type.");
    }

    // get the number of events
    size_t num_events = variable->getValue().size();
    if ( num_events == 0 )
    {
    	abort = true;
    	return RbConstants::Double::neginf;
    }

    // choose the event index
    size_t event_index = size_t(rng->uniform01() * num_events);

    // get the events
    const std::map<double, valueType>& events = variable->getValue().getEvents();
    typename std::map<double, valueType>::const_iterator this_event = events.begin();
    std::advance(this_event, event_index);

    // get the value
    event_time = this_event->first;
    old_value  = this_event->second;

    // choose the value to change
    size_t element_index = size_t(rng->uniform01() * old_value.size());

    // perform a Scale proposal
	double u = rng->uniform01();
    double scaling_factor = std::exp( lambda * ( u - 0.5 ) );
	valueType new_value = old_value;
	new_value[element_index] *= scaling_factor;

	// update the value in the variable
	abort = !variable->getValue().changeEvent(event_time, new_value);
	if ( abort == true )
	{
		return RbConstants::Double::neginf;
	}

	// touch the variable
	variable->touch(true);

	return std::log(scaling_factor);
}


/**
 *
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::prepareProposal( void )
{
    
}


/**
 * Print the summary of the Proposal.
 *
 * The summary just contains the current value of the tuning parameter.
 * It is printed to the stream that it passed in.
 *
 * \param[in]     o     The stream to which we print the summary.
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::printParameterSummary(std::ostream &o, bool name_only) const
{
    // nothing to print
    
}


/**
 * Reject the Proposal.
 *
 * Since the Proposal stores the previous value and it is the only place
 * where complex undo operations are known/implement, we need to revert
 * the value of the variable/DAG-node to its original value.
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::undoProposal( void )
{
	if ( abort == false )
	{
	    variable->getValue().changeEvent(event_time, old_value);
	    variable->touch(true);
	}
}


/**
 * Swap the current variable for a new one.
 *
 * \param[in]     oldN     The old variable that needs to be replaced.
 * \param[in]     newN     The new RevVariable.
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::swapNodeInternal(DagNode *oldN, DagNode *newN)
{
    
    variable = static_cast< StochasticNode<OrderedEvents<valueType> >* >(newN) ;
    
}


template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::setProposalTuningParameter(double tp)
{
    // this proposal has no tuning parameter: nothing to do
	lambda = tp;
}


/**
 * Tune the Proposal to accept the desired acceptance ratio.
 *
 * The acceptance ratio for this Proposal should be around 0.44.
 * If it is too large, then we increase the proposal size,
 * and if it is too small, then we decrease the proposal size.
 */
template <class valueType>
void RevBayesCore::OrderedEventVectorScaleProposal<valueType>::tune( double rate )
{
    double p = this->targetAcceptanceRate;
    if ( rate > p )
    {
        lambda *= (1.0 + ((rate-p)/(1.0 - p)) );
    }
    else
    {
    	lambda /= (2.0 - rate/p);
    }
    
    lambda = fmin(10000, lambda);
}



#endif
