#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "AbstractPhyloContinuousCharacterProcess.h"
#include "BranchHistory.h"
#include "ConstantNode.h"
#include "DistributionNormal.h"
#include "PhyloOrnsteinUhlenbeckStateDependent.h"
#include "RandomNumberFactory.h"
#include "RbException.h"
#include "StochasticNode.h"
#include "TopologyNode.h"
#include "ContinuousCharacterData.h"
#include "ContinuousTaxonData.h"
#include "RbConstants.h"
#include "RbVector.h"
#include "RbVectorImpl.h"
#include "StandardState.h"
#include "StringUtilities.h"
#include "Tree.h"
#include "TreeChangeEventHandler.h"
#include "TreeHistoryCtmc.h"
#include "TypedDagNode.h"

namespace RevBayesCore { class DagNode; }
namespace RevBayesCore { class RandomNumberGenerator; }


using namespace RevBayesCore;

PhyloOrnsteinUhlenbeckStateDependent::PhyloOrnsteinUhlenbeckStateDependent(const TypedDagNode<CharacterHistoryDiscrete> *ch, size_t ns, ROOT_TREATMENT rt) : TypedDistribution< ContinuousCharacterData > ( new ContinuousCharacterData() ),
    num_nodes( ch->getValue().getNumberBranches()+1 ),
    num_sites( ns ),
    partial_likelihoods( std::vector<std::vector<std::vector<double> > >(2, std::vector<std::vector<double> >(this->num_nodes, std::vector<double>(this->num_sites, 0) ) ) ),
    means( std::vector<std::vector<std::vector<double> > >(2, std::vector<std::vector<double> >(this->num_nodes, std::vector<double>(this->num_sites, 0) ) ) ),
    variances( std::vector<std::vector<double> >(2, std::vector<double>(this->num_nodes, 0) ) ),
    active_likelihood( std::vector<size_t>(this->num_nodes, 0) ),
    changed_nodes( std::vector<bool>(this->num_nodes, false) ),
    dirty_nodes( std::vector<bool>(this->num_nodes, true) ),
    character_histories( ch )
{
    // initialize default parameters
    root_state                  = new ConstantNode<double>("", new double(0.0) );
    homogeneous_alpha           = new ConstantNode<double>("", new double(0.0) );
    homogeneous_sigma           = new ConstantNode<double>("", new double(1.0) );
    homogeneous_theta           = new ConstantNode<double>("", new double(0.0) );
    state_dependent_alpha       = NULL;
    state_dependent_sigma       = NULL;
    state_dependent_theta       = NULL;
    root_treatment              = rt;
    
    
    // add parameters
    addParameter( homogeneous_alpha );
    addParameter( homogeneous_sigma );
    addParameter( homogeneous_theta );
    addParameter( character_histories );
    addParameter( root_state );
    
    // now we need to reset the value
    this->redrawValue();
    
    // we need to reset the contrasts
    resetValue();
}


/**
 * Destructor. Because we added ourselves as a reference to tau when we added a listener to its
 * TreeChangeEventHandler, we need to remove ourselves as a reference and possibly delete tau
 * when we die. All other parameters are handled by others.
 */
PhyloOrnsteinUhlenbeckStateDependent::~PhyloOrnsteinUhlenbeckStateDependent( void )
{
    // We don't delete the params, because they might be used somewhere else too. The model needs to do that!
    
//    // remove myself from the tree listeners
//    if ( tau != NULL )
//    {
//        tau->getValue().getTreeChangeEventHandler().removeListener( this );
//    }
    
}



PhyloOrnsteinUhlenbeckStateDependent* PhyloOrnsteinUhlenbeckStateDependent::clone( void ) const
{
    
    return new PhyloOrnsteinUhlenbeckStateDependent( *this );
}



double PhyloOrnsteinUhlenbeckStateDependent::computeBranchTime( size_t nide_idx, double brlen )
{
    
    // get the clock rate for the branch
    double branch_time = 1.0;
//    if ( this->heterogeneous_clock_rates != NULL )
//    {
//        double sigma = this->heterogeneous_clock_rates->getValue()[nide_idx];
//        branch_time = sigma * sigma * brlen;
//    }
//    else
//    {
//        double sigma = this->homogeneous_clock_rate->getValue();
//        branch_time = sigma * sigma * brlen;
//    }
    
    return branch_time;
}


double PhyloOrnsteinUhlenbeckStateDependent::computeStateDependentAlpha(size_t state_idx) const
{
    
    // get the selection rate for the branch
    double a;
    if ( this->state_dependent_alpha != NULL )
    {
        a = this->state_dependent_alpha->getValue()[state_idx];
    }
    else
    {
        a = this->homogeneous_alpha->getValue();
    }
    
    return a;
}


double PhyloOrnsteinUhlenbeckStateDependent::computeStateDependentSigma(size_t state_idx) const
{
    
    // get the drift rate for the branch
    double s;
    if ( this->state_dependent_sigma != NULL )
    {
        s = this->state_dependent_sigma->getValue()[state_idx];
    }
    else
    {
        s = this->homogeneous_sigma->getValue();
    }
    
    return s;
}


double PhyloOrnsteinUhlenbeckStateDependent::computeStateDependentTheta(size_t state_idx) const
{
    
    // get the optimum (theta) for the branch
    double t;
    if ( this->state_dependent_theta != NULL )
    {
        t = this->state_dependent_theta->getValue()[state_idx];
    }
    else
    {
        t = this->homogeneous_theta->getValue();
    }
    
    return t;
}


double PhyloOrnsteinUhlenbeckStateDependent::computeRootState( size_t state_index ) const
{
    
    // get the root-state parameter
    double root_state_value = this->root_state->getValue();
    
    return root_state_value;
}


double PhyloOrnsteinUhlenbeckStateDependent::computeLnProbability( void )
{
    
//    // we need to check here if we still are listining to this tree for change events
//    // the tree could have been replaced without telling us
//    if ( tau->getValue().getTreeChangeEventHandler().isListening( this ) == false )
//    {
//        tau->getValue().getTreeChangeEventHandler().addListener( this );
//        dirty_nodes = std::vector<bool>(tau->getValue().getNumberOfNodes(), true);
//    }
    
    const Tree& tau = character_histories->getValue().getTree();
    
    // compute the ln probability by recursively calling the probability calculation for each node
    const TopologyNode &root = tau.getRoot();
    
    // we start with the root and then traverse down the tree
    size_t root_index = root.getIndex();
        
    // only necessary if the root is actually dirty
    if ( this->dirty_nodes[root_index] )
    {
                
        recursiveComputeLnProbability( root, root_index );
        
        // sum the partials up
        this->ln_prob = sumRootLikelihood();
        
    }
    //std::cout << this->ln_prob << std::endl;
    return this->ln_prob;
}



//void PhyloOrnsteinUhlenbeckStateDependent::fireTreeChangeEvent( const TopologyNode &n, const unsigned& m )
//{
//
//    // call a recursive flagging of all node above (closer to the root) and including this node
//    recursivelyFlagNodeDirty( n );
//
//}


void PhyloOrnsteinUhlenbeckStateDependent::keepSpecialization( const DagNode* affecter )
{
    
    // reset all flags
    for (std::vector<bool>::iterator it = this->dirty_nodes.begin(); it != this->dirty_nodes.end(); ++it)
    {
        (*it) = false;
    }
    
    for (std::vector<bool>::iterator it = this->changed_nodes.begin(); it != this->changed_nodes.end(); ++it)
    {
        (*it) = false;
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::recursiveComputeLnProbability( const TopologyNode &node, size_t node_index )
{
    
    // check for recomputation
    if ( node.isTip() == false && dirty_nodes[node_index] == true )
    {
        // mark as computed
        dirty_nodes[node_index] = false;
        
        std::vector<double> &p_node       = this->partial_likelihoods[this->active_likelihood[node_index]][node_index];
        std::vector<double> &mu_node                   = this->means[this->active_likelihood[node_index]][node_index];

        
        
        // get the number of children
        size_t num_children = node.getNumberOfChildren();
        
        for (size_t j = 1; j < num_children; ++j)
        {
            
            size_t left_index = node_index;
            const TopologyNode *left = &node;
            if ( j == 1 )
            {
                left        = &node.getChild(0);
                left_index  = left->getIndex();
                recursiveComputeLnProbability( *left, left_index );
            }
                        
            const TopologyNode &right   = node.getChild(j);
            size_t right_index          = right.getIndex();
            recursiveComputeLnProbability( right, right_index );
            
            const std::vector<double> &p_left  = this->partial_likelihoods[this->active_likelihood[left_index]][left_index];
            const std::vector<double> &p_right = this->partial_likelihoods[this->active_likelihood[right_index]][right_index];
            
            // get the means for the left and right subtrees
            const std::vector<double> &mu_left  = this->means[this->active_likelihood[left_index]][left_index];
            const std::vector<double> &mu_right = this->means[this->active_likelihood[right_index]][right_index];
            
            // get the variances of the left and right child nodes
            double delta_left  = this->variances[this->active_likelihood[left_index]][left_index];
            double delta_right = this->variances[this->active_likelihood[right_index]][right_index];
            
            
            // @TODO: maybe change later to allow for more characters/sites.
            double mean_left  = mu_left[0];
            double mean_right = mu_right[0];
            
            // get character history for tree
            const CharacterHistory& current_history = character_histories->getValue();
            
            // get branch history for the left branch
            const BranchHistory& bh_left = current_history.getHistory(left_index);
            const std::multiset<CharacterEvent*,CharacterEventCompare>& hist_left = bh_left.getHistory();
            
            double v_left;
            double log_nf_left = 0;
            double begin_time_left = left->getAge();
            for (std::multiset<CharacterEvent*,CharacterEventCompare>::const_iterator it=hist_left.begin(); it!=hist_left.end(); ++it)
            {
                //std::cout << "hello\t" << std::endl;
                CharacterEventDiscrete* event = static_cast<CharacterEventDiscrete*>(*it);
                double event_time = event->getAge();
                // we need to find the current state
                size_t current_state = event->getState();
                
                double time_left = event_time - begin_time_left;
                double sigma_left = computeStateDependentSigma( current_state );
                double alpha_left = computeStateDependentAlpha( current_state );
                double theta_left = computeStateDependentTheta( current_state );
               
                if ( alpha_left > 1E-20 )
                {
                    v_left  = (sigma_left*sigma_left) / (2.0*alpha_left) * (exp(2.0*alpha_left*time_left) - 1.0 );
                    mean_left  = exp(1.0 * time_left  * alpha_left ) * (mean_left  - theta_left)  + theta_left;
                }
                else
                {
                    v_left  = (sigma_left*sigma_left) * time_left;
                    //mean_left  = current_mu_left; // no change
                }
                delta_left = v_left + delta_left * exp(2.0*alpha_left *time_left);
                begin_time_left = event_time;
                
                // update the log normalizing factor
                log_nf_left += time_left * alpha_left;
            }
            size_t last_state_left  = static_cast<CharacterEventDiscrete*>(bh_left.getParentCharacters()[0])->getState();
            
            
            // get branch history for the right branch
            const BranchHistory& bh_right = current_history.getHistory(right_index);
            const std::multiset<CharacterEvent*,CharacterEventCompare>& hist_right = bh_right.getHistory();

            double v_right;
            double log_nf_right = 0;
            double begin_time_right = right.getAge();
            for (std::multiset<CharacterEvent*,CharacterEventCompare>::const_iterator it=hist_right.begin(); it!=hist_right.end(); ++it)
            {
                CharacterEventDiscrete* event = static_cast<CharacterEventDiscrete*>(*it);
                double event_time = event->getAge();
                
                // we need to set the current rate category
                size_t current_state = event->getState();
                
                double time_right = event_time - begin_time_right;
                double sigma_right = computeStateDependentSigma( current_state );
                double alpha_right = computeStateDependentAlpha( current_state );
                double theta_right = computeStateDependentTheta( current_state );
               
                if ( alpha_right > 1E-20 )
                {
                    v_right = (sigma_right*sigma_right) / (2.0*alpha_right) * (exp(2.0*alpha_right*time_right) - 1.0 );
                    mean_right   = exp(1.0 * time_right  * alpha_right ) * (mean_right  - theta_right)  + theta_right;
                }
                else
                {
                    v_right = (sigma_right*sigma_right) * time_right;
                    //mean_right = current_mu_right; no change
                }
                delta_right = v_right + delta_right * exp(2.0*alpha_right *time_right);
               
                // update the time
                begin_time_right     = event_time;
                // update the log normalizing factor
                log_nf_right += time_right * alpha_right;
            }
            size_t last_state_right  = static_cast<CharacterEventDiscrete*>(bh_right.getParentCharacters()[0])->getState();


            // the above code does not run if there are no transitions. In either case, we need to finish the "final" branch segment
            double bl_left = node.getAge() - begin_time_left;
            double sigma_left = computeStateDependentSigma(last_state_left);
            double alpha_left = computeStateDependentAlpha(last_state_left);
            double theta_left = computeStateDependentTheta(last_state_left);
            if ( alpha_left > 1E-20 )
            {
                v_left = (sigma_left*sigma_left) / (2.0*alpha_left) * (exp(2.0*alpha_left*bl_left) - 1.0 );
                mean_left  = exp(1.0 * bl_left  * alpha_left ) * (mean_left  - theta_left)  + theta_left;
            }
            else
            {
                v_left = (sigma_left*sigma_left) * bl_left;
                //mean_left = nothing to change
            }
            delta_left = v_left + delta_left * exp(2.0*alpha_left *bl_left);
            log_nf_left += bl_left * alpha_left;
            
            // same but for right branch
            double bl_right = node.getAge() - begin_time_right;
            double sigma_right = computeStateDependentSigma(last_state_right);
            double alpha_right = computeStateDependentAlpha(last_state_right);
            double theta_right = computeStateDependentTheta(last_state_right);
            if ( alpha_right > 1E-20 )
            {
                v_right = (sigma_right*sigma_right) / (2.0*alpha_right) * (exp(2.0*alpha_right*bl_right) - 1.0 );
                mean_right  = exp(1.0 * bl_right  * alpha_right ) * (mean_right  - theta_right)  + theta_right;
            }
            else
            {
                v_right = (sigma_right*sigma_right) * bl_right;
                //mean_right = nothing to change
            }
            delta_right = v_right + delta_right * exp(2.0*alpha_right *bl_right);
            log_nf_right += bl_right * alpha_right;

            double var_left = delta_left;
            double var_right = delta_right;
            
            // calculate and store the node variance
            double var_node = (var_left*var_right) / (var_left+var_right);
            this->variances[this->active_likelihood[node_index]][node_index] = var_node;


            // this loop is broken currently, does not work for multiple characters
            for (int i=0; i<this->num_sites; i++)
            {
                mu_node[i] = (var_left*mean_right + var_right*mean_left) / (var_left+var_right);
                
                // compute the contrasts for this site and node
                double contrast = mean_left - mean_right;
                
                double a = -(contrast*contrast / (2*(var_left + var_right)));               
                double b = log(2*RbConstants::PI*(var_left+var_right))/2.0;
                double lnl_node = log_nf_left + log_nf_right + a - b;
                
                if ( node.isRoot() == true )
                {
                    double var_root;
                    double root_state;
                    if (root_treatment == OPTIMUM)
                    {
                        double theta = computeStateDependentTheta ( last_state_left );
                        var_root = var_node;
                        root_state = theta;
                    }
                    else if (root_treatment == EQUILIBRIUM)
                    {
                        double theta = computeStateDependentTheta(last_state_left);
                        double sigma = computeStateDependentSigma(last_state_left);
                        double alpha = computeStateDependentAlpha(last_state_left);
                        double stationary_variance = sigma * sigma / (2 * alpha);
                        root_state = theta;
                        var_root = var_node + stationary_variance;
                    }
                    else if (root_treatment == PARAMETER)
                    {
                        root_state = computeRootState( last_state_left );
                        var_root = var_node;
                    }
                    lnl_node += RbStatistics::Normal::lnPdf( root_state, sqrt(var_root), mu_node[i]);
                }
                // sum up the log normalizing factors of the subtrees
                p_node[i] = lnl_node + p_left[i] + p_right[i];
                
            } // end for-loop over all sites
            
        } // end for-loop over all children
        
    } // end if we need to compute something for this node.
    
}



void PhyloOrnsteinUhlenbeckStateDependent::recursivelyFlagNodeDirty( const TopologyNode &n )
{
    
    // we need to flag this node and all ancestral nodes for recomputation
    size_t index = n.getIndex();
    
    // if this node is already dirty, the also all the ancestral nodes must have been flagged as dirty
    if ( !dirty_nodes[index] )
    {
        // the root doesn't have an ancestor
        if ( !n.isRoot() )
        {
            recursivelyFlagNodeDirty( n.getParent() );
        }
        
        // set the flag
        dirty_nodes[index] = true;
        
        // if we previously haven't touched this node, then we need to change the active likelihood pointer
        if ( changed_nodes[index] == false )
        {
            active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
            changed_nodes[index] = true;
        }
        
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::redrawValue(void)
{
    // delete the old value first
    delete this->value;
    
    // create a new character data object
    this->value = new ContinuousCharacterData();
    
    // create a vector of taxon data
    std::vector< ContinuousTaxonData > taxa = std::vector< ContinuousTaxonData >( num_nodes, ContinuousTaxonData( Taxon("") ) );
    
    const Tree& tau = character_histories->getValue().getTree();
    
    // simulate the root sequence
    size_t root_index = tau.getRoot().getIndex();
    ContinuousTaxonData &root = taxa[ root_index ];
    
    std::vector<double> root_states = simulateRootCharacters(num_sites);
    for ( size_t i = 0; i < num_sites; ++i )
    {
        // create the character
        double c = root_states[i];
        
        // add the character to the sequence
        root.addCharacter( c );
    }
    
    // recursively simulate the sequences
    simulateRecursively( tau.getRoot(), taxa );
    
    // we call now our method to resample the tips
    // this is important if we have multiple samples (e.g. individuals) per species
    simulateTipSamples( taxa );
    
    // tell the derived classes
    this->resetValue();

}



void PhyloOrnsteinUhlenbeckStateDependent::resetValue( void )
{
    
    // check if the vectors need to be resized
    partial_likelihoods  = std::vector<std::vector<std::vector<double> > >(2, std::vector<std::vector<double> >(this->num_nodes, std::vector<double>(this->num_sites, 0) ) );
    means                = std::vector<std::vector<std::vector<double> > >(2, std::vector<std::vector<double> >(this->num_nodes, std::vector<double>(this->num_sites, 0) ) );
    variances            = std::vector<std::vector<double> >(2, std::vector<double>(this->num_nodes, 0) );


    // create a vector with the correct site indices
    // some of the sites may have been excluded
    std::vector<size_t> site_indices = std::vector<size_t>(this->num_sites,0);
    size_t site_index = 0;
    for (size_t i = 0; i < this->num_sites; ++i)
    {
        while ( this->value->isCharacterExcluded(site_index) )
        {
            ++site_index;
            if ( site_index >= this->value->getNumberOfCharacters()  )
            {
                throw RbException( "The character matrix cannot set to this variable because it does not have enough included characters." );
            }
        }
        site_indices[i] = site_index;
        ++site_index;
    }
    
    const Tree& tau = character_histories->getValue().getTree();
    std::vector<TopologyNode*> nodes = tau.getNodes();
    for (size_t site = 0; site < this->num_sites; ++site)
    {
        
        for (std::vector<TopologyNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        {
            if ( (*it)->isTip() )
            {
                ContinuousTaxonData& taxon = this->value->getTaxonData( (*it)->getName() );
                double &c = taxon.getCharacter(site_indices[site]);
                means[0][(*it)->getIndex()][site] = c;
                means[1][(*it)->getIndex()][site] = c;
                variances[0][(*it)->getIndex()] = 0;
                variances[1][(*it)->getIndex()] = 0;
            }
        }
    }
    
    
    // finally we set all the flags for recomputation
    for (std::vector<bool>::iterator it = dirty_nodes.begin(); it != dirty_nodes.end(); ++it)
    {
        (*it) = true;
    }
    
    // flip the active likelihood pointers
    for (size_t index = 0; index < changed_nodes.size(); ++index)
    {
        active_likelihood[index] = 0;
        changed_nodes[index] = true;
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::restoreSpecialization( const DagNode* affecter )
{
    
    // reset the flags
    for (std::vector<bool>::iterator it = dirty_nodes.begin(); it != dirty_nodes.end(); ++it)
    {
        (*it) = false;
    }
    
    // restore the active likelihoods vector
    for (size_t index = 0; index < changed_nodes.size(); ++index)
    {
        // we have to restore, that means if we have changed the active likelihood vector
        // then we need to revert this change
        if ( changed_nodes[index] == true )
        {
            active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
        }
        
        // set all flags to false
        changed_nodes[index] = false;
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setAlpha(const TypedDagNode<double> *a)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_alpha );
    this->removeParameter( state_dependent_alpha );
    homogeneous_alpha       = NULL;
    state_dependent_alpha   = NULL;
    
    
    // set the value
    homogeneous_alpha = a;
    
    // add the new parameter
    this->addParameter( homogeneous_alpha );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setAlpha(const TypedDagNode<RbVector<double> > *a)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_alpha );
    this->removeParameter( state_dependent_alpha );
    homogeneous_alpha       = NULL;
    state_dependent_alpha   = NULL;
    
    
    // set the value
    state_dependent_alpha   = a;
    
    // add the new parameter
    this->addParameter( state_dependent_alpha );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setRootState(const TypedDagNode<double> *s)
{
    
    // remove the old parameter first
    this->removeParameter( root_state );
    root_state = s;
    
    // add the new parameter
    this->addParameter( root_state );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setSigma(const TypedDagNode<double> *s)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_sigma );
    this->removeParameter( state_dependent_sigma );
    homogeneous_sigma       = NULL;
    state_dependent_sigma   = NULL;
    
    
    // set the value
    homogeneous_sigma = s;
    
    // add the new parameter
    this->addParameter( homogeneous_sigma );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setSigma(const TypedDagNode<RbVector<double> > *s)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_sigma );
    this->removeParameter( state_dependent_sigma );
    homogeneous_sigma       = NULL;
    state_dependent_sigma   = NULL;
    
    
    // set the value
    state_dependent_sigma = s;
    
    // add the new parameter
    this->addParameter( state_dependent_sigma );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setTheta(const TypedDagNode<double> *t)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_theta );
    this->removeParameter( state_dependent_theta );
    homogeneous_theta        = NULL;
    state_dependent_theta    = NULL;
    
    
    // set the value
    homogeneous_theta = t;
    
    // add the new parameter
    this->addParameter( homogeneous_theta );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setTheta(const TypedDagNode<RbVector<double> > *t)
{
    
    // remove the old parameter first
    this->removeParameter( homogeneous_theta );
    this->removeParameter( state_dependent_theta );
    homogeneous_theta        = NULL;
    state_dependent_theta    = NULL;
    
    
    // set the value
    state_dependent_theta = t;
    
    // add the new parameter
    this->addParameter( state_dependent_theta );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
    
}


void PhyloOrnsteinUhlenbeckStateDependent::setValue(ContinuousCharacterData *v, bool force)
{
    
    // delegate to the parent class
    TypedDistribution< ContinuousCharacterData >::setValue(v, force);
    
    // reset the number of sites
    num_sites = v->getNumberOfIncludedCharacters();
    
    // tell the derived classes
    this->resetValue();
    
}

double PhyloOrnsteinUhlenbeckStateDependent::simulateEpisode(size_t state_index, double delta_t, double ancestral_value)
{
    RandomNumberGenerator* rng = GLOBAL_RNG;

    // get the parameter values
    double sigma = computeStateDependentSigma(state_index);
    double theta = computeStateDependentTheta(state_index);
    double alpha = computeStateDependentAlpha(state_index);

    // calculate the mean
    double e = exp(-alpha * delta_t);
    double mu = e * (ancestral_value - theta) + theta;

    // calculate the variance
    double sd;
    if (alpha > 1e-10){
        double stationary_var = (sigma * sigma) / (2 * alpha); 
        double var = stationary_var * (1.0 - e * e); 
        sd = sqrt(var);
    }else{
        sd = sigma * sqrt(delta_t);
    }

    // draw the new character state as a Gaussian random variable
    double y = RbStatistics::Normal::rv(mu, sd, *rng);

    return y;
}


void PhyloOrnsteinUhlenbeckStateDependent::simulateRecursively( const TopologyNode &node, std::vector< ContinuousTaxonData > &taxa)
{

    // get the children of the node
    const std::vector<TopologyNode*>& children = node.getChildren();
    
    // get the sequence of this node
    size_t node_index = node.getIndex();
    const ContinuousTaxonData &parent = taxa[ node_index ];
    
    // simulate the sequence for each child
    for (std::vector< TopologyNode* >::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        const TopologyNode &child = *(*it);
        
        // get the branch length for this child
        double branch_length = child.getBranchLength();
        
        // get the branch specific rate
        double branch_time = computeBranchTime( child.getIndex(), branch_length );
        const CharacterHistory& current_history = character_histories->getValue();
        size_t child_index = child.getIndex();
        const BranchHistory& bh = current_history.getHistory(child_index);

        const std::multiset<CharacterEvent*,CharacterEventCompare>& history = bh.getHistory();

        // compute the first episode
        //double begin_time = child.getAge();


        ContinuousTaxonData &taxon = taxa[ child.getIndex() ];
        // loop over the number of characters
        for ( size_t i = 0; i < num_sites; ++i )
        {
            double youngest_time = child.getAge();
            double begin_time = youngest_time;

            // the episode states and times (if there was at least one discrete character state change)
            // the loop is from young to old
            // since it's pushed to the front of the deque,
            // the array is in order of old to young

            std::deque<double> times;
            std::deque<size_t> states;

            for (std::multiset<CharacterEvent*, CharacterEventCompare>::const_iterator iter = history.begin(); iter != history.end(); ++iter)
            {
                // get the state change event
                CharacterEventDiscrete* event = static_cast<CharacterEventDiscrete*>(*iter);    

                // calculate the times
                double event_time = event->getAge();
                double delta_t = event_time - begin_time;
                begin_time = event_time;

                // get the state index
                size_t current_state = event->getState();

                // save the 
                times.push_front(delta_t);
                states.push_front(current_state);
            }

            // do it again, since the iterator above only does n-1 of the episodes
            size_t first_state = static_cast<CharacterEventDiscrete*>(bh.getParentCharacters()[0])->getState();
            size_t first_delta_t = node.getAge() - begin_time;

            times.push_front(first_delta_t);
            states.push_front(first_state);

            // get the ancestral character for this site
            double y = parent.getCharacter( i );

            // simulate the episodes
            for (size_t j = 0; j < times.size(); ++j){
                double state = states[j];
                double delta_t = times[j];
                y = PhyloOrnsteinUhlenbeckStateDependent::simulateEpisode(state, delta_t, y);
            }

            taxon.addCharacter(y);
        }
        
        if ( child.isTip() )
        {
            taxon.setTaxon( child.getTaxon() );
        }
        else
        {
            // recursively simulate the sequences
            simulateRecursively( child, taxa );
        }
        
    }
    
}


std::vector<double> PhyloOrnsteinUhlenbeckStateDependent::simulateRootCharacters(size_t n)
{
    RandomNumberGenerator* rng = GLOBAL_RNG;
    
    std::vector<double> chars = std::vector<double>(num_sites, 0);

    double theta, sigma, alpha, stationary_variance;
    if (root_treatment == OPTIMUM || root_treatment == EQUILIBRIUM){

        const Tree& tau = character_histories->getValue().getTree();
        const TopologyNode &root = tau.getRoot();
        const TopologyNode *left = &root.getChild(0);
        size_t left_index = left->getIndex();
        const CharacterHistory& left_history = character_histories->getValue();
        const BranchHistory& bh_left = left_history.getHistory(left_index);
        size_t root_state_index  = static_cast<CharacterEventDiscrete*>(bh_left.getParentCharacters()[0])->getState();
        theta = computeStateDependentTheta(root_state_index);
        sigma = computeStateDependentSigma(root_state_index);
        alpha = computeStateDependentAlpha(root_state_index);
        stationary_variance = sigma * sigma / (2 * alpha);
    }

    for (size_t i=0; i<num_sites; ++i)
    {
        if (root_treatment == OPTIMUM)
        {
            chars[i] = theta;
        }
        else if (root_treatment == EQUILIBRIUM)
        {
            double y = RbStatistics::Normal::rv(theta, sqrt(stationary_variance), *rng);
            chars[i] = y;
        }
        else if (root_treatment == PARAMETER)
        {
            chars[i] = computeRootState( 0 );
        }
        else
        {
        throw RbException( "the rootTreatment has not been set properly" );
        }
    }
    
    return chars;
}


void PhyloOrnsteinUhlenbeckStateDependent::simulateTipSamples( const std::vector< ContinuousTaxonData > &taxon_data )
{
    
    const Tree& tau = character_histories->getValue().getTree();
    // add the taxon data to the character data
    for (size_t i = 0; i < tau.getNumberOfTips(); ++i)
    {
        this->value->addTaxonData( taxon_data[i] );
    }
    
}


double PhyloOrnsteinUhlenbeckStateDependent::sumRootLikelihood( void )
{
    // get the root node
    const Tree& tau = character_histories->getValue().getTree();
    const TopologyNode &root = tau.getRoot();
    
    // get the index of the root node
    size_t node_index = root.getIndex();
    
    // get the pointers to the partial likelihoods of the left and right subtree
    std::vector<double> &p_node = this->partial_likelihoods[this->active_likelihood[node_index]][node_index];
    
    // sum the log-likelihoods for all sites together
    double sum_partial_probs = 0.0;
    for (size_t site = 0; site < this->num_sites; ++site)
    {
        sum_partial_probs += p_node[site];
    }
    
    return sum_partial_probs;
}


void PhyloOrnsteinUhlenbeckStateDependent::touchSpecialization( const DagNode* affecter, bool touchAll )
{
    const Tree& tau = character_histories->getValue().getTree();

    // if the topology wasn't the culprit for the touch, then we just flag everything as dirty
    if ( affecter == this->state_dependent_sigma && false )
    {
        
        const std::set<size_t> &indices = this->state_dependent_sigma->getTouchedElementIndices();
        
        // maybe all of them have been touched or the flags haven't been set properly
        if ( indices.size() == 0 )
        {
            // just flag everyting for recomputation
            touchAll = true;
        }
        else
        {
            const std::vector<TopologyNode *> &nodes = tau.getNodes();
            // flag recomputation only for the nodes
            for (std::set<size_t>::iterator it = indices.begin(); it != indices.end(); ++it)
            {
                this->recursivelyFlagNodeDirty( *nodes[*it] );
            }
        }
    }
    else // if the topology wasn't the culprit for the touch, then we just flag everything as dirty
    {
        touchAll = true;
        
        if ( affecter == this->dag_node )
        {
            resetValue();
        }
        
    }
    
    if ( touchAll )
    {
        for (std::vector<bool>::iterator it = dirty_nodes.begin(); it != dirty_nodes.end(); ++it)
        {
            (*it) = true;
        }
        
        // flip the active likelihood pointers
        for (size_t index = 0; index < changed_nodes.size(); ++index)
        {
            if ( changed_nodes[index] == false )
            {
                active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
                changed_nodes[index] = true;
            }
        }
        
    }
    
}


/** Swap a parameter of the distribution */
void PhyloOrnsteinUhlenbeckStateDependent::swapParameterInternal(const DagNode *oldP, const DagNode *newP)
{
    
    if (oldP == root_state)
    {
        root_state = static_cast<const TypedDagNode< double >* >( newP );
    }
    
    if (oldP == homogeneous_alpha)
    {
        homogeneous_alpha = static_cast<const TypedDagNode< double >* >( newP );
    }
    else if (oldP == state_dependent_alpha)
    {
        state_dependent_alpha = static_cast<const TypedDagNode< RbVector< double > >* >( newP );
    }
    
    if (oldP == homogeneous_sigma)
    {
        homogeneous_sigma = static_cast<const TypedDagNode< double >* >( newP );
    }
    else if (oldP == state_dependent_sigma)
    {
        state_dependent_sigma = static_cast<const TypedDagNode< RbVector< double > >* >( newP );
    }
    
    if (oldP == homogeneous_theta)
    {
        homogeneous_theta = static_cast<const TypedDagNode< double >* >( newP );
    }
    else if (oldP == state_dependent_theta)
    {
        state_dependent_theta = static_cast<const TypedDagNode< RbVector< double > >* >( newP );
    }
    
    if (oldP == character_histories)
    {
        character_histories = static_cast<const TypedDagNode< CharacterHistoryDiscrete >* >( newP );
    }
        
}

