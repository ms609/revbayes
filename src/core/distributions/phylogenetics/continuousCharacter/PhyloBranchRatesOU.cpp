#include "PhyloBranchRatesOU.h"

#include <stddef.h>
#include <cmath>

#include "DistributionNormal.h"
#include "RandomNumberFactory.h"
#include "Cloner.h"
#include "RbVectorImpl.h"
#include "Tree.h"
#include "TypedDagNode.h"



using namespace RevBayesCore;



// constructor(s)
PhyloBranchRatesOU::PhyloBranchRatesOU(const TypedDagNode< Tree > *tr, const TypedDagNode< double >* ro, const TypedDagNode< double >* si, const TypedDagNode< double >* al, const TypedDagNode< double >* th): TypedDistribution< RbVector<double> >(new RbVector<double>(tr->getValue().getNumberOfNodes()-1,0.0)),
    tau( tr ),
    root_state( ro ),
    sigma( si ),
    alpha( al ),
    theta( th )
{
    // add the parameters to our set (in the base class)
    // in that way other class can easily access the set of our parameters
    // this will also ensure that the parameters are not getting deleted before we do
    addParameter( tau );
    addParameter( root_state );
    addParameter( sigma );
    addParameter( alpha );
    addParameter( theta );
    
    simulate();
}



PhyloBranchRatesOU* PhyloBranchRatesOU::clone(void) const
{
    return new PhyloBranchRatesOU( *this );
}


double PhyloBranchRatesOU::computeLnProbability(void)
{
    size_t n_nodes = tau->getValue().getNumberOfNodes();
    std::vector<double> node_values = std::vector<double>(n_nodes, 0.0);
    if ( this->value->size() != (n_nodes-1) )
    {
        throw RbException("The dimension of the rates vector and the tree don't match.");
    }
    node_values[n_nodes-1] = root_state->getValue();
    double ln_prob = recursiveLnProb(tau->getValue().getRoot(), node_values);
    
    ln_prob += (n_nodes-1) * RbConstants::LN2;

    return ln_prob;
}


double PhyloBranchRatesOU::recursiveLnProb( const TopologyNode& node, std::vector<double> &parent_values )
{
    
    double ln_prob = 0.0;
    size_t index = node.getIndex();
    
    if ( node.isRoot() == false )
    {
        
        // x ~ normal(x_up, sigma^2 * branchLength)
        size_t parent_index    = node.getParent().getIndex();
        double parent_value    = parent_values[parent_index];
        double ln_parent_value = log( parent_value );
        double branch_rate = (*this->value)[ index ];
        double node_value = 2*branch_rate - parent_value;
        if ( node_value < 0.0 )
        {
            return RbConstants::Double::neginf;
        }
        double ln_node_value = log(node_value);
        double t = node.getBranchLength();
        double e = exp(-alpha->getValue() * t);
        double e2 = exp(-2 * alpha->getValue() * t);
        double ln_mean = e * ln_parent_value + (1 - e) * theta->getValue();
        double stand_dev = sigma->getValue() * sqrt((1 - e2) / 2 / alpha->getValue());
        
        ln_prob += RbStatistics::Normal::lnPdf(ln_node_value, stand_dev, ln_mean) - ln_node_value;
        
        parent_values[index] = node_value;
    }
    
    // propagate forward
    size_t num_children = node.getNumberOfChildren();
    
    for (size_t i = 0; i < num_children; ++i)
    {
        ln_prob += recursiveLnProb(node.getChild(i), parent_values);
    }
    
    return ln_prob;
    
}

void PhyloBranchRatesOU::redrawValue(void)
{
    simulate();
}


void PhyloBranchRatesOU::simulate()
{
    
    size_t n_nodes = tau->getValue().getNumberOfNodes();
    std::vector<double> node_values = std::vector<double>(n_nodes, 0.0);
    node_values[n_nodes-1] = root_state->getValue();
    recursiveSimulate(tau->getValue().getRoot(), node_values);
}


void PhyloBranchRatesOU::recursiveSimulate(const TopologyNode& node, std::vector<double> &parent_values)
{
    
    size_t index = node.getIndex();
    
    if ( node.isRoot() == false )
    {
        
        // x ~ normal(x_up, sigma^2 * branchLength)
        
        size_t parent_index = node.getParent().getIndex();

        double ln_parent_value = log(parent_values[parent_index]);
        double t = node.getBranchLength();
        double e  = exp(-alpha->getValue() * t);
        double e2 = exp(-2 * alpha->getValue() * t);
        double ln_mean = e * ln_parent_value + (1 - e) * theta->getValue();
        double stand_dev = sigma->getValue() * sqrt((1 - e2) / 2 / alpha->getValue());

        // simulate the new Val
        RandomNumberGenerator* rng = GLOBAL_RNG;
        parent_values[index] = exp(RbStatistics::Normal::rv( ln_mean, stand_dev, *rng));
        (*this->value)[index] = (parent_values[parent_index] + parent_values[index]) / 2.0;
                
    }
    
    // propagate forward
    size_t num_children = node.getNumberOfChildren();
    for (size_t i = 0; i < num_children; ++i)
    {
        recursiveSimulate(node.getChild(i), parent_values);
    }
    
}

/** Swap a parameter of the distribution */
void PhyloBranchRatesOU::swapParameterInternal(const DagNode *oldP, const DagNode *newP)
{
    
    if ( oldP == tau )
    {
        tau = static_cast< const TypedDagNode<Tree> * >( newP );
    }
    
    if ( oldP == root_state )
    {
        root_state = static_cast< const TypedDagNode<double> * >( newP );
    }
    
    if ( oldP == sigma )
    {
        sigma = static_cast< const TypedDagNode<double> * >( newP );
    }
    
    if ( oldP == alpha )
    {
        alpha = static_cast< const TypedDagNode< double > * >( newP );
    }
    
    if ( oldP == theta )
    {
        theta = static_cast< const TypedDagNode< double > * >( newP );
    }
    
}
