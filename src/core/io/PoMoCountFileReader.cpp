#include <cstddef>
#include <string>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

#include "DiscreteTaxonData.h"
#include "PoMoCountFileReader.h"
#include "PoMoState.h"
#include "Cloneable.h"
#include "DelimitedDataReader.h"
#include "HomologousDiscreteCharacterData.h"
#include "NaturalNumbersState.h"
#include "RbException.h"
#include "StringUtilities.h"
#include "Taxon.h"


using namespace RevBayesCore;


PoMoCountFileReader::PoMoCountFileReader(const path &fn, const size_t vps, FORMAT f) : DelimitedDataReader(fn, ""),
    virtual_population_size ( vps ),
    data_format( f )
{
    if ( data_format == FORMAT::PoMo )
    {
        matrix = new HomologousDiscreteCharacterData<PoMoState>();
    }
    else
    {
        matrix = new HomologousDiscreteCharacterData<NaturalNumbersState>();
    }

	// chars is a matrix containing all the lines of the file fn.
	// First line, with the names of the columns:
	// First line should be like: COUNTSFILE  NPOP 5   NSITES N

	int start = -1;
	// Skip comments.
	do {
			start = start + 1;
	}
	while (chars[start][0] == "#");

	if (chars[start][0] != "COUNTSFILE" || chars[0].size() != 5)
    {
        throw RbException()<<"File "<<fn<<" is not a proper PoMo Counts file: first line is not correct, it should be similar to \nCOUNTSFILE NPOP 5 NSITES N\n.";
	}
	else
    {
        number_of_populations = StringUtilities::asIntegerNumber( chars[0][2] );
        number_of_sites = StringUtilities::asIntegerNumber( chars[0][4] );
	}
	size_t numberOfFields = 2 + number_of_populations;

	// The second line should look like this:
	//CHROM  POS  Sheep    BlackSheep  RedSheep  Wolf     RedWolf
	if (chars[start+1][0] != "CHROM" || chars[1][1] != "POS" || chars[1].size() != numberOfFields)
        {
            throw RbException()<<"File "<<fn<<" is not a proper PoMo Counts file: second line is not correct, it should be similar to \nCHROM POS Sheep BlackSheep RedSheep Wolf RedWolf\n.";
	}
	else
    {
		for (size_t i = start+2; i < 2 + number_of_populations; ++i )
        {
			names.push_back(chars[1][i]);
		}
	}

	// Setting the taxon names in the data matrix
	std::map<std::string, DiscreteTaxonData<PoMoState> > name_to_taxon_data;
	for (size_t i = 0; i < names.size(); ++i )
    {
		DiscreteTaxonData<PoMoState> tax (names[i]);
		name_to_taxon_data.insert(std::pair< std::string, DiscreteTaxonData<PoMoState> >(names[i], tax) );
	}


    // estimate the number of states
    size_t row=2;
    size_t col=2;
    // move ahead until we found a non-missing state
    while ( chars[row][col] == "?" || chars[row][col] == "-" )
    {
        ++col;
        if ( col == chars[row].size() )
        {
            // reset at end of row
            ++row;
            col = 2;
        }
        if ( row == chars.size() )
        {
            throw RbException("Couldn't guess the number of states in PoMo counts file because all states missing.");
        }
    }
    std::vector<std::string> tmp_res;
    StringUtilities::stringSplit(chars[row][col], ",", tmp_res);
    size_t num_states = tmp_res.size();

	for (size_t i = 2; i < chars.size(); ++i)
	{
            
        if (chars[i].size() != numberOfFields)
        {
            throw RbException()<<"File "<<fn<<" is not a proper PoMo Counts file: line "<<i<<" is not correct, it does not have "<<numberOfFields<<" space-separated fields.";
        }

		const std::string& chromosome = chars[i][0];
		size_t position = StringUtilities::asIntegerNumber( chars[i][1] );

		for (size_t j = 2; j < 2 + number_of_populations; ++j)
		{

            if ( num_states == 4 )
            /* got an error here when num_sates=4
            stringvalue: 6,0,0,0
1 1e-08 1e-08 1e-08 0.015625 0.015625 0.015625 1e-08 1e-08 1e-08 

 
terminate called after throwing an instance of 'std::bad_alloc'
  what():  std::bad_alloc
Aborted (core dumped)

            */
            {
                std::cout << "§1: " << chars[i][j] << " - " << chromosome << " - " << position << " - " << virtual_population_size  << "\n\n"; 
                PoMoState pState (4, virtual_population_size, chars[i][j], chromosome, position );
                std::cout << "§2: " << names[j-2] << "\n\n"; 
                name_to_taxon_data.at(names[j-2]).addCharacter( pState);
//                pState.setWeighted(true);
                pState.setWeighting(PoMoState::BINOMIAL);
//                std::cout << pState.getWeighting() << "\n\n";
                pState.setState(chars[i][j]);
                std::vector<double> vec = pState.getWeights();
                std::cout << "§3: " << "Weights " << pState.isWeighted() << " Size " << vec.size() << " String Value "<< pState.getStringValue() << "\n\n";
                for (int i=0; i < vec.size(); i++)
                {
                    std::cout << vec[i] << ' ';
                }
                std::cout << "Reached here!\n\n";

            }
            else
            {
                PoMoState pState (num_states, virtual_population_size, chars[i][j], chromosome, position );
                name_to_taxon_data.at(names[j-2]).addCharacter( pState);
            }
		}
	}

	// We have finished all lines, we fill up the data matrix
    for (std::map<std::string, DiscreteTaxonData<PoMoState> >::iterator tax = name_to_taxon_data.begin(); tax != name_to_taxon_data.end(); ++tax )
    {
        matrix->addTaxonData(tax->second);
    }





}

PoMoCountFileReader::PoMoCountFileReader(const PoMoCountFileReader& r) : DelimitedDataReader(r),
number_of_populations(r.number_of_populations),
number_of_sites(r.number_of_sites),
virtual_population_size(r.virtual_population_size),
names(r.names),
data_format(r.data_format)
{
    matrix = r.matrix->clone();
}


PoMoCountFileReader::~PoMoCountFileReader()
{
    delete matrix;
}

PoMoCountFileReader& PoMoCountFileReader::PoMoCountFileReader::operator=(const PoMoCountFileReader& r)
{
    if (this != &r)
    {
        number_of_populations   = r.number_of_populations;
        number_of_sites         = r.number_of_sites;
        virtual_population_size = r.virtual_population_size;
        names                   = r.names;
        data_format             = r.data_format;
        
        matrix                  = r.matrix->clone();
    }
    
    return *this;
}

const size_t PoMoCountFileReader::getNumberOfPopulations( void )
{
	return number_of_populations;
}

const size_t PoMoCountFileReader::getNumberOfSites( void )
{
	return number_of_sites;
}

const AbstractHomologousDiscreteCharacterData& PoMoCountFileReader::getMatrix( void )
{

	return *matrix;

}

const size_t PoMoCountFileReader::getVirtualPopulationSize( void )
{
	return virtual_population_size;
}
