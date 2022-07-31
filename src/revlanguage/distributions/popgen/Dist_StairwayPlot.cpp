#include <math.h>
#include <stddef.h>
#include <ostream>
#include <string>
#include <vector>

#include "ArgumentRule.h"
#include "ArgumentRules.h"
#include "Dist_StairwayPlot.h"
#include "StairwayPlotDistribution.h"
#include "RbVector.h"
#include "RlSimplex.h"
#include "ConstantNode.h"
#include "DagMemberFunction.h"
#include "DagNode.h"
#include "DeterministicNode.h"
#include "DistributionMemberFunction.h"
#include "DynamicNode.h"
#include "IndirectReferenceFunction.h"
#include "ModelObject.h"
#include "ModelVector.h"
#include "Natural.h"
#include "RbHelpReference.h"
#include "RevObject.h"
#include "RevPtr.h"
#include "RevVariable.h"
#include "RlConstantNode.h"
#include "RlDagMemberFunction.h"
#include "RlDeterministicNode.h"
#include "RlDistributionMemberFunction.h"
#include "RlStochasticNode.h"
#include "RlTypedDistribution.h"
#include "RlTypedFunction.h"
#include "StochasticNode.h"
#include "TypeSpec.h"
#include "TypedDagNode.h"
#include "TypedDistribution.h"
#include "TypedFunction.h"
#include "UserFunctionNode.h"

namespace RevBayesCore { class Simplex; }

using namespace RevLanguage;

Dist_StairwayPlot::Dist_StairwayPlot(void) : TypedDistribution< ModelVector<Natural> >()
{
    
}


Dist_StairwayPlot::~Dist_StairwayPlot(void)
{
    
}



Dist_StairwayPlot* Dist_StairwayPlot::clone( void ) const
{
    
    return new Dist_StairwayPlot(*this);
}


RevBayesCore::StairwayPlotDistribution* Dist_StairwayPlot::createDistribution( void ) const
{
    
    // get the parameters
    long                                                            n_sites = static_cast<const Natural              &>( num_sites->getRevObject() ).getValue();
    long                                                            n_ind   = static_cast<const Natural              &>( num_individuals->getRevObject() ).getValue();
    long                                                            f       = static_cast<const RlBoolean            &>( folded->getRevObject() ).getValue();
    RevBayesCore::TypedDagNode< RevBayesCore::RbVector<double> >*   th      = static_cast<const ModelVector<RealPos> &>( theta->getRevObject() ).getDagNode();
    RevBayesCore::StairwayPlotDistribution*                         d       = new RevBayesCore::StairwayPlotDistribution( th, n_sites, n_ind, f );
    return d;
}



/* Get Rev type of object */
const std::string& Dist_StairwayPlot::getClassType(void)
{
    
    static std::string rev_type = "Dist_StairwayPlot";
    return rev_type;
}

/* Get class type spec describing type of object */
const TypeSpec& Dist_StairwayPlot::getClassTypeSpec(void)
{
    
    static TypeSpec rev_type_spec = TypeSpec( getClassType(), new TypeSpec( TypedDistribution< ModelVector<Natural> >::getClassTypeSpec() ) );
    return rev_type_spec;
}


/**
 * Get the Rev name for the distribution.
 * This name is used for the constructor and the distribution functions,
 * such as the density and random value function
 *
 * \return Rev name of constructor function.
 */
std::string Dist_StairwayPlot::getDistributionFunctionName( void ) const
{
    // create a distribution name variable that is the same for all instance of this class
    std::string d_name = "StairwayPlot";
    
    return d_name;
}


MethodTable Dist_StairwayPlot::getDistributionMethods( void ) const
{
    MethodTable methods = TypedDistribution< ModelVector<Natural> >::getDistributionMethods();
    
    ArgumentRules* times_arg_rules = new ArgumentRules();
    methods.addFunction( new DistributionMemberFunction<Dist_StairwayPlot, ModelVector<RealPos> >( "getTimes", variable, times_arg_rules, true, true ) );

    return methods;
}



/** Return member rules (no members) */
const MemberRules& Dist_StairwayPlot::getParameterRules(void) const
{
    
    static MemberRules dist_member_rules;
    static bool rules_set = false;
    
    if ( rules_set == false )
    {
        dist_member_rules.push_back( new ArgumentRule( "theta", ModelObject<RealPos>::getClassTypeSpec(), "The theta values with theta=4*Ne*mu. We expect n-1 theta values where n is the number of individuals.", ArgumentRule::BY_CONSTANT_REFERENCE, ArgumentRule::ANY ) );
        dist_member_rules.push_back( new ArgumentRule( "numSites", Natural::getClassTypeSpec(), "The number of sites in the SFS.", ArgumentRule::BY_CONSTANT_REFERENCE, ArgumentRule::ANY ) );
        dist_member_rules.push_back( new ArgumentRule( "numIndividuals", Natural::getClassTypeSpec(), "The number of individuals in (unfolded) the SFS.", ArgumentRule::BY_CONSTANT_REFERENCE, ArgumentRule::ANY ) );
        dist_member_rules.push_back( new ArgumentRule( "folded", RlBoolean::getClassTypeSpec(), "Is the site frequency folded.", ArgumentRule::BY_CONSTANT_REFERENCE, ArgumentRule::ANY, new RlBoolean(false) ) );

        rules_set = true;
    }
    
    return dist_member_rules;
}


const TypeSpec& Dist_StairwayPlot::getTypeSpec( void ) const
{
    
    static TypeSpec ts = getClassTypeSpec();
    return ts;
}


/** Print value for user */
void Dist_StairwayPlot::printValue(std::ostream& o) const
{
    
    o << "StairwayPlot(theta=";
    if ( theta != NULL )
        o << theta->getName();
    else
        o << "?";
    o << ")";
}


/** Set a member variable */
void Dist_StairwayPlot::setConstParameter(const std::string& name, const RevPtr<const RevVariable> &var)
{
    
    if ( name == "theta" )
    {
        theta = var;
    }
    else if ( name == "numSites" )
    {
        num_sites = var;
    }
    else if ( name == "numIndividuals" )
    {
        num_individuals = var;
    }
    else if ( name == "folded" )
    {
        folded = var;
    }
    else
    {
        TypedDistribution< ModelVector<Natural> >::setConstParameter(name, var);
    }
}
