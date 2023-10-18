#ifndef SlideProposal_H
#define SlideProposal_H

#include <iosfwd>

#include "SimpleProposal.h"

namespace RevBayesCore {
class DagNode;
template <class variableType> class StochasticNode;
    
    /**
     * The sliding operator.
     *
     * A sliding proposal draws a random uniform number u ~ unif (-0.5,0.5)
     * and slides the current vale by a sliding offset
     * delta  = ( lambda * u )
     * where lambda is the tuning parameter of the proposal to influence the size of the proposals.
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @since 2009-09-08, version 1.0
     *
     */
    class SlideProposal : public SimpleProposal<double> {
        
    public:
        SlideProposal( StochasticNode<double> *n, double l, double p=0.44);                                                                      //!<  constructor
        
        // Basic utility functions
        void                                    cleanProposal(void);                                                                //!< Clean up proposal
        SlideProposal*                          clone(void) const;                                                                  //!< Clone object
        const std::string&                      getProposalName(void) const;                                                        //!< Get the name of the proposal for summary printing
        double                                  getProposalTuningParameter(void) const;
        void                                    printParameterSummary(std::ostream &o, bool name_only) const;                                       //!< Print the parameter summary
        void                                    prepareProposal(void);                                                              //!< Prepare the proposal
        double                                  propose(double &v);                                                                  //!< Perform proposal
        void                                    setProposalTuningParameter(double tp);
        void                                    tune(double r);                                                                     //!< Tune the proposal to achieve a better acceptance/rejection ratio
        void                                    undoProposal(void);                                                                 //!< Reject the proposal
        
    protected:
        void                                    swapNodeInternal(DagNode *oldN, DagNode *newN);                                     //!< Swap the DAG nodes on which the Proposal is working on
        
    private:
        
        // parameters
        double                                  storedValue;                                                                        //!< The stored value of the Proposal used for rejections.
        double                                  lambda;                                                      //!< The value we propose.
    };
    
}

#endif

