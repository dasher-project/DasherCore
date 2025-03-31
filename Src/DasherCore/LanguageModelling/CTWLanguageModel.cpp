// CTWLanguageModel.cpp
//


//
// For information on the CTW method visit
// http://www.sps.ele.tue.nl/members/F.M.J.Willems/RESEARCH_files/CTW/ResearchCTW.htm
//

//#include "stdafx.h"
#include "CTWLanguageModel.h"
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include "HashTable.h"

using namespace Dasher;


CCTWLanguageModel::CCTWLanguageModel(int iNumSyms) : CLanguageModel(iNumSyms) {

	Dasher::CHashTable HashTable;  // create hashtable
	MaxDepth = 6;   // Maximum depth of the context tree
	MaxTries = 15;	// Max. number of attempts to find a spot in the hash table
	alpha = 14;		// 2: KT-estimator, 1: Laplace estimator, 14 = found by P.A.J. Volf to be 'good' for text
	MaxNrNodes = 4194304; // Max number of CCTWNodes in the table, trade-off between compression and memory usage. 2^22 = 4M
    TotalNodes = 0; // to keep track of how many nodes are created in the table.
	MaxFill = 0.9f;  // Threshold to decide when to freeze the tree
	Failed = 0;		// keep track of how many nodes couldn't be found or created //debug
	Frozen = false; // to indicate if there is still room in the array of CCTWNodes
	MaxCount = 255; // Maximum value for the counts for count-halving
	NrBits = 9;    // number of bits used for representation of probabilities
	MaxValue = (1<<NrBits) -1;

    NrPhases = (int)ceil(log((double)(GetSize()))/log(2.0)); // number of bits per input-symbol
	Tree = new CCTWNode[MaxNrNodes]; // create array with all CCTWNodes.

	// Fill RootIndex table with indices of the RootNodes <- now I round up to next power of 2, should only create for possible symbols
	// Does that make a noticable difference in memory usage? Rootnodes with no symbols associated will accumulate no counts, so they only cost 1 node each (6 bytes).
	// Does it waste codespace? Do rootnodes with no symbols associated with them still get assigned a positive probability?
	// Set NrTries to max+1, to identify a RootNode
	for (int i = 0; i<(1<<NrPhases);i++)
    {
     RootIndex[i] = HashTable.GetHashOffSet(i) & (MaxNrNodes-1); // MaxNrNodes is a power of 2, & results in a mod operation, walk 'round' through the array
	 Tree[RootIndex[i]].NrTries = MaxTries+1; // in rootnodes the character value doesn't matter, as long as Tries = unique
	 TotalNodes++;
    }
}

CCTWLanguageModel::~ CCTWLanguageModel(){ // destructor
	delete [] Tree;
}

// **** Implementation of help functions *****
// To get the 'phase' bit in byte
#define ByteBit(byte,Phase)	((byte >> ((NrPhases-1)-Phase)) & 1)

// To find the index of the RootNode for a byte and a phase.
inline int CCTWLanguageModel::MapIndex(int b, int f){
	return ((1<<f)-1 + (b>>(NrPhases-f)));  //(2^phase -1) + dec. value of most significant bits
}

inline void CCTWLanguageModel::Scale(uint64_t &a, uint64_t &b)
{
	// Instead of using the full 16 bits for the probabilities, use only 9,
	// that's the only relevant information the other bits are noise <- depends on the value of MaxCount,
	// should use log2(alpha) bits more than for the counts a and b

	while ((a > MaxValue) | (b > MaxValue))
	{ // scale to right
		a = a>>1;
		b = b>>1;
	}
	while ((a < (MaxValue>>1)) & (b < (MaxValue>>1)))
	{// scale to left
		a = a<<1;
		b = b<<1;
	}
	//prevent 0
	if (a == 0)
	{
		a = 1;
	}
	if (b == 0)
	{
		b = 1;
	}
}

void CCTWLanguageModel::UpdatePath(int bit, int Update, int ValidDepth, int* & index, unsigned short int & P0, unsigned short int & P1)
{ // updates the CTW data of the nodes in 'index' with value of 'bit'.
  // Update specifies yes (1) or no (0) (GetProbs). In the case 'no', the new Pws are calculated but the tree is not
  // altered in any way

	uint64_t GammaZero;  		// (GammaZero / (GammaZero + GammaOne)) = Pw(0|x)
	uint64_t GammaOne;   		// (GammaOne  / (GammaZero + GammaOne)) = Pw(1|x)
	unsigned short int CountZero; // Number of zeros seen so far in this node
	unsigned short int CountOne;  // Number of ones seen so far in this node
	uint64_t PeBlockZero;    		// Local block probability of sequence (0,x)
	uint64_t PeBlockOne; 	  	// Local block probability of sequence (1,x)
	uint64_t PwCBlockZero;     	// Product of the weighted block probabilities of the childnodes of sequence (0,x)
	uint64_t PwCBlockOne;      	// Product of the weighted block probabilities of the childnodes of sequence (1,x)
	uint64_t PeCondZero; 		// Conditional local probability (0|x)
	uint64_t PeCondOne;  		// Conditional local probability (1|x)
	uint64_t PwCBlock;      		// Product of the weighted block probabilities of the childnodes of sequence (x)
	uint64_t PeBlock;	  		// Local block probability of sequence (x)

	// The deepest index can be a leaf, a failed node, or a not-placed node
	const int DeepestIndex = index[ValidDepth];

	if (DeepestIndex == MaxNrNodes) // node didn't exist yet, both probs. equal
	{
		GammaZero = MaxValue;
		GammaOne  = MaxValue;
	}
	else if (DeepestIndex == MaxNrNodes+1) // node couldn't be placed
	{   // could do more fancy things here
		GammaZero = MaxValue;
		GammaOne  = MaxValue;
	}
	else
	{ // node has to be a leaf
		CountZero = Tree[DeepestIndex].a;
		CountOne  = Tree[DeepestIndex].b;

		GammaZero = alpha*CountZero +1;
		GammaOne  = alpha*CountOne  +1;

		if (Update == 1) // update tree
		{// first update counts
			if(bit)
			{
				if (CountOne == MaxCount)
				{ // half counts
					CountZero = (CountZero+1) / 2;
					CountOne = (CountOne+1) / 2;
				}
				else
					CountOne++;
			}
			else // bit = 0
			{
				if (CountZero == MaxCount)
				{ // half counts
					CountZero = (CountZero+1) / 2;
					CountOne = (CountOne+1) / 2;
				}
				else
					CountZero++;
			}
			Tree[DeepestIndex].a = static_cast<unsigned char>(CountZero);
			Tree[DeepestIndex].b = static_cast<unsigned char>(CountOne);
		}	// end if update
	} // end if/else, deepest index done
	// now all the internal nodes, including the rootnode
	for(int i=ValidDepth-1;i>=0;i--)
	{
		CountZero = Tree[index[i]].a;
		CountOne  = Tree[index[i]].b;

		PwCBlock = Tree[index[i]].PwChild;
		PeBlock  = Tree[index[i]].Pe;

		PeCondZero = (alpha*CountZero)+1;
		PeCondOne =  (alpha*CountOne) +1;
		PeBlockZero = PeBlock*PeCondZero*(GammaOne+GammaZero);
	    PeBlockOne  = PeBlock*PeCondOne*(GammaOne+GammaZero);
		PwCBlockZero = PwCBlock*GammaZero*((alpha*(CountZero+CountOne))+2);
		PwCBlockOne  = PwCBlock*GammaOne *((alpha*(CountZero+CountOne))+2);

		GammaZero = (PeBlockZero + PwCBlockZero);
		GammaOne  = (PeBlockOne  + PwCBlockOne );

		Scale(GammaZero, GammaOne);

		if (Update == 1) // update tree
		{// first update counts
			if(bit)
			{
				if (CountOne == MaxCount)
				{ // half counts
					CountZero = (CountZero+1) / 2;
					CountOne = (CountOne+1) / 2;
				}
				else
					CountOne++;

				Scale(PeBlockOne, PwCBlockOne);
				Tree[index[i]].Pe = static_cast<unsigned short>(PeBlockOne); // conversion after scaling, no problem
				Tree[index[i]].PwChild = static_cast<unsigned short>(PwCBlockOne);
			}
			else // bit = 0
			{
				if (CountZero == MaxCount)
				{ // half counts
					CountZero = (CountZero+1) / 2;
					CountOne = (CountOne+1) / 2;
				}
				else
					CountZero++;

				Scale(PeBlockZero, PwCBlockZero);
				Tree[index[i]].Pe = static_cast<unsigned short>(PeBlockZero);
				Tree[index[i]].PwChild = static_cast<unsigned short>(PwCBlockZero);
			}
			Tree[index[i]].a = static_cast<unsigned char>(CountZero);
			Tree[index[i]].b = static_cast<unsigned char>(CountOne);
		} // end if update
	}
	P0 = static_cast<unsigned short>(GammaZero); // Gammas are already scaled back to 16 bits
	P1 = static_cast<unsigned short>(GammaOne);
}

int CCTWLanguageModel::FindPath(CCTWContext & context, char NewChar, int phase, int create, int* & index)
{ // Puts the Tree-array indices of the CCTWNodes on the path of Context in index.
  // Returns the depth till which the path is found, index[] deeper than that is garbage!
  // If 'create' = 1, new nodes are created when an empty spot is found
    int Stepsize = 0;
    int curindex = RootIndex[MapIndex(NewChar,phase)];   // Find root, depending on current (newest) character in context
	index[0] = curindex;

	// From the root, find/create the nodes, corresponding to the context
    for (unsigned int i=0; i<context.Context.size();i++)
    {
	  unsigned char CurChar = context.Context.at(i);
	  Stepsize = (CHashTable::GetHashOffSet(CurChar)<<1)+1; // get stepsize. Shift+1 to keep result odd, to prevent cycles
      bool found = false;
      for (int Tries = 1; Tries<MaxTries; Tries++)
      {
        curindex = (curindex + Stepsize) & (MaxNrNodes-1);

		if (Tree[curindex].NrTries == Tries) // node in use, is it the correct node?
        {// see if this is the correct node: compare tries and last (current) character
          if (Tree[curindex].Symbol == CurChar) // node found
          {
            found = true;				// to avoid 'failed'
			index[i+1] = curindex;      // tell calling function where to find the node, i+1 because index[0] = rootnode
			break;						// to escape loop and continue with next character
          }
        }
        if (Tree[curindex].NrTries == 0) // empty node found, create new node
        {
		  if (!create) // No need to create a new node, let calling function know empty spot found
		  {
			  index[i+1] = MaxNrNodes; // to indicate node could be placed but didn't exist yet
			  return (i+1); // +1 since i=0 is the rootnode, always valid
		  }
          if (!Frozen) // Still space in the tree
		  {
			  Tree[curindex].NrTries = Tries;
			  Tree[curindex].Symbol = CurChar;
			  TotalNodes++;				// new node in use
			  if ((float)(TotalNodes)/(float)(MaxNrNodes) > MaxFill) // Max fillratio of tree reached, freeze tree
				  Frozen = true;
			  found = true;					// to avoid 'failed'
			  index[i+1] = curindex;		// tell calling function where to find the node, i+1 because index[0] = rootnode
			  break;						// to escape loop and continue with next character
		  } else // can't create a new node
		  {
			  found = false;
			  index[i+1] = MaxNrNodes+1;	// to indicate node could not be placed
			  return (i+1);					// +i since i=0 is the rootnode, always valid
		  }
        } // else collision, set next step
      } // for Tries
      // after MaxTries attempts:
      if (!found) // check to see if we were succesfull
      { // apparently, character could not be placed
		index[i+1] =  MaxNrNodes+1;			// to indicate node could not be placed
		return i+1;				// indicate node could not be found/created for this phase
      } //if !found
    } // for i contextsize
return static_cast<int>(context.Context.size()); // all nodes on the path found/created
} // end findpath


// **** Implementation of interface functions  *****

void CCTWLanguageModel::EnterSymbol(Context CurContext, int Symbol)
{ // add Symbol to the front of Context. If there are more than MaxDepth symbols, pop the last one
	CCTWLanguageModel::CCTWContext &Context = *(CCTWLanguageModel::CCTWContext *) (CurContext);
	if (Context.Full == true)
		Context.Context.pop_back();

	Context.Context.push_front(Symbol);

	if (Context.Context.size() == MaxDepth)
		Context.Full = true;
}

void CCTWLanguageModel::LearnSymbol(Context CurContext, int Symbol)
{
  CCTWLanguageModel::CCTWContext &Context = *(CCTWLanguageModel::CCTWContext *) (CurContext);

  if (Context.Full == true) // context is complete, update the tree
  {	// find indices of the tree nodes corresponding to the context

	int *Index = new int[Context.Context.size()+1]; // +1 for the rootnode
	int ValidDepth = 0;
	for (int phase = 0;phase<NrPhases;phase++)
	{
		ValidDepth = FindPath(Context, Symbol, phase, 1, Index); // Find indices of the nodes for this phase and context
		// nodes on the path for this phase found, update the tree
		unsigned short int stubZ =0;
		unsigned short int stubO =0;
		UpdatePath(ByteBit(Symbol,phase), 1, ValidDepth, Index, stubZ, stubO);
	}
	delete [] Index;

	Context.Context.pop_back();     // only delete last symbol if context is complete
  }
  Context.Context.push_front(Symbol); // update context with newest symbol

  if (Context.Context.size() == MaxDepth)
		Context.Full = true;
}

void CCTWLanguageModel::GetProbs(Context context, std::vector<unsigned int> &Probs, int Norm, int iUniform) const
{   	// because we reuse findpath and updatepath function, we need to de-const the object :(
	// findpath should be declared const anyway (?)

	CCTWLanguageModel* self = const_cast<CCTWLanguageModel*>(this);

	CCTWContext *CTWContext = ( CCTWContext *)(context);
	CCTWContext LocalContext(*CTWContext);

	int iNumSymbols = GetSize();
	int MinProb = iUniform / iNumSymbols; //smallest probability to assign

	Probs.resize(iNumSymbols);
	int pLeft = 0;

	// calculate probabilities of all possible symbols. Again assume all 2^NrPhases
	int *Index = new int[LocalContext.Context.size()+1]; // +1 for the rootnode

	vector <unsigned short int>Interval((1<<(NrPhases+1))-1); // number of rootnodes*2 (1 prob for bit 0 and 1 each)
	if (Norm>65535)
	{
		Interval[0]=65535; // to prevent overflow
	    pLeft = Norm-65535; // if Norm is way bigger than 2^16 - 1, uniformly distributing the 'leftover' could still cause overflow
	}
	else
		Interval[0] = Norm;

	int ValidDepth = 0;
	uint64_t IntervalB = 0; // 'base' interval
	uint64_t IntervalZ = 0; // divided interval for the 0-branch
	uint64_t IntervalO = 0; // divided interval for the 1-branch
	unsigned int MinInterval = 0;
	unsigned short int Pw0 = 0;
	unsigned short int Pw1 = 0;

	for (int phase = 0;phase<NrPhases;phase++)
	{
		int stepsize = 1 <<(NrPhases-phase); // 2^maxphase-1 - fase
		for (int steps = 0;steps < 1<<phase;steps++)
		{ // find the path for all needed symbols
			// FIXME now I round up to next power of 2
			ValidDepth = self->FindPath(LocalContext, steps*stepsize, phase, 0, Index); // Find indices of the nodes for this phase and context

			IntervalB = Interval[(1<<phase)+ steps - 1];
			self->UpdatePath(0,0, ValidDepth, Index, Pw0, Pw1);

			IntervalZ = (IntervalB * Pw0)/(uint64_t)(Pw0+Pw1); // flooring, influence of flooring P0 instead of P1 is negligible
			IntervalO = IntervalB - IntervalZ;

			MinInterval = MinProb*1<<(NrPhases-1-phase); // leafs for each rootnode at the current phase, assuming a full alphabet!!

			//make sure all leafs from this point will get at least probability 1
			if(IntervalZ < MinInterval)
			{
				IntervalO = IntervalO - (MinInterval-IntervalZ);
				IntervalZ = IntervalZ + (MinInterval-IntervalZ);
			}
			else if(IntervalO < MinInterval)
			{
				IntervalZ = IntervalZ - (MinInterval-IntervalO);
				IntervalO = IntervalO + (MinInterval-IntervalO);
			}

			Interval[(1<<(phase+1))+ 2*steps - 1] = static_cast<unsigned short int>(IntervalZ);
			Interval[(1<<(phase+1))+ 2*steps] = static_cast<unsigned short int>(IntervalO);
		} // for steps
	} // for phase
	delete [] Index;

	// Copy the intervals associated with the actual symbols to the vector Probs.
	Probs.assign((Interval.end()-(1ULL<NrPhases)), (Interval.end()-(1ULL<<NrPhases)+iNumSymbols));
	pLeft +=Probs[0]; //symbol 0 is a special dummy symbol, should get prob. 0
	Probs[0] = 0;

	// calculate how many extra symbols exist in tree, because iNumSymbols is not a power of 2
    int Extra = (1<<NrPhases) - iNumSymbols;
	// take the probabilities from non-existing symbols and re-divide it over existing symbols
	for (int j = Extra; j >0; j-- ) {
		pLeft +=Interval[Interval.size()-j];
	}

	int iLeft = iNumSymbols-1; //divide the probability that is left over the symbols
	for(int j = 1; j < iNumSymbols; ++j) {
		unsigned int p = pLeft / iLeft;
		Probs[j] += p;
		--iLeft;
		pLeft -= p;
	}

} // end function GetProbs


bool CCTWLanguageModel::WriteToFile(std::string strFilename, std::string AlphabetName){
	SLMFileHeader GenericHeader;
	GenericHeader.iAlphabetSize = GetSize(); // Number of characters in the alphabet
	GenericHeader.iHeaderVersion = 1; // Version of the header
	GenericHeader.iLMID = 5; // ID of the language model, 5 for CTW
	GenericHeader.iLMMinVersion = 1; //Minimum backwards compatible version for the language model
	GenericHeader.iLMVersion = 1; // Version number of the language model, version 1 is the stored hashtable, april 2007
	GenericHeader.iHeaderSize = sizeof(SLMFileHeader) + static_cast<unsigned short>(AlphabetName.length()); // Total size of header (including variable length alphabet name)

	FILE *OutputFile;
	OutputFile = fopen(strFilename.c_str(), "wb");
	if(OutputFile)
	{
		// write header
		fwrite(GenericHeader.szMagic , sizeof(GenericHeader.szMagic[0]), sizeof(GenericHeader.szMagic) - 1, OutputFile); //Do not print Null-Char
		fwrite(&GenericHeader.iHeaderVersion, sizeof(GenericHeader.iHeaderVersion), 1, OutputFile);
		fwrite(&GenericHeader.iHeaderSize, sizeof(GenericHeader.iHeaderSize), 1, OutputFile);
		fwrite(&GenericHeader.iLMID, sizeof(GenericHeader.iLMID), 1, OutputFile);
		fwrite(&GenericHeader.iLMVersion, sizeof(GenericHeader.iLMVersion), 1, OutputFile);
		fwrite(&GenericHeader.iLMMinVersion, sizeof(GenericHeader.iLMMinVersion), 1, OutputFile);
		fwrite(&GenericHeader.iAlphabetSize, sizeof(GenericHeader.iAlphabetSize), 1, OutputFile);
		fwrite(AlphabetName.c_str(), sizeof(AlphabetName[0]), AlphabetName.length(), OutputFile); // UTF-8 encoded alphabet name (variable length struct)

		// CTW specific, not in SLMFileHeader
		fwrite(&MaxNrNodes, sizeof(MaxNrNodes), 1, OutputFile);

		for(int i=0;i<MaxNrNodes;i++)
		{
			fwrite(&Tree[i].a, sizeof(CCTWNode::a), 1, OutputFile);
			fwrite(&Tree[i].b, sizeof(CCTWNode::b), 1, OutputFile);
			fwrite(&Tree[i].Symbol, sizeof(CCTWNode::Symbol), 1,OutputFile);
			fwrite(&Tree[i].NrTries, sizeof(CCTWNode::NrTries),1,OutputFile);
			fwrite(&Tree[i].Pe, sizeof(CCTWNode::Pe),1,OutputFile);
			fwrite(&Tree[i].PwChild, sizeof(CCTWNode::PwChild),1,OutputFile);
		}
		fclose(OutputFile);
		return true;
	}
	else return false;
}

bool CCTWLanguageModel::ReadFromFile(std::string strFilename, std::string AlphabetName){
	FILE *InputFile;
	InputFile = fopen(strFilename.c_str(), "rb");
	if(InputFile)
	{
		/* Read and check header, close file and return failure when header is not what we expect.
		TODO: Checking of the SLMFileHeader, which is not specific to the CTW languagemodel should be done in DasherModel,
		only CTW specific information (MaxNrNodes) should be checked here.
		The values to compare with should be parameters and not hardcoded. */

		SLMFileHeader GenericHeader;
		char* ReadAlphabetName;

		size_t bytesRead;

		bytesRead = fread(&GenericHeader.szMagic, sizeof(GenericHeader.szMagic[0]), sizeof(GenericHeader.szMagic) - 1, InputFile); //Magic string is written without null-char
		if(bytesRead < (sizeof(GenericHeader.szMagic) - 1) || memcmp(GenericHeader.szMagic,"%DLF",bytesRead))
		{ // magic strings not equal
			return false;
		}

		bytesRead = fread(&GenericHeader.iHeaderVersion, sizeof(GenericHeader.iHeaderVersion), 1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iHeaderVersion) || GenericHeader.iHeaderVersion != 1)
		{ // unknown header version
			return false;
		}

		bytesRead = fread(&GenericHeader.iHeaderSize,sizeof(GenericHeader.iHeaderSize),1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iHeaderSize)) return false; //Not enough bytes read

		bytesRead = fread(&GenericHeader.iLMID,sizeof(GenericHeader.iLMID),1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iLMID) || GenericHeader.iLMID != 5)
		{ // header indicates this is not a CTW model
			return false;
		}

		bytesRead = fread(&GenericHeader.iLMVersion,sizeof(GenericHeader.iLMVersion),1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iLMVersion)) return false; //Not enough bytes read

		bytesRead = fread(&GenericHeader.iLMMinVersion,sizeof(GenericHeader.iLMMinVersion),1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iLMMinVersion) || GenericHeader.iLMMinVersion > 1)
		{ // header indicates stored model newer than we can handle
			return false;
		}

		bytesRead = fread(&GenericHeader.iAlphabetSize,sizeof(GenericHeader.iAlphabetSize),1, InputFile);
		if(bytesRead < sizeof(GenericHeader.iAlphabetSize) || GenericHeader.iAlphabetSize != GetSize())
		{ // header indicates stored model uses an alphabet of different size
			return false;
		}

		ReadAlphabetName = new char[GenericHeader.iHeaderSize - sizeof(SLMFileHeader) + 1];
		bytesRead = fread(ReadAlphabetName,sizeof(ReadAlphabetName[0]), GenericHeader.iHeaderSize - sizeof(SLMFileHeader), InputFile);
		if(bytesRead < GenericHeader.iHeaderSize - sizeof(SLMFileHeader)) return false; //Not enough bytes read
		ReadAlphabetName[GenericHeader.iHeaderSize - sizeof(SLMFileHeader)] = '\0'; // write the terminating 0 and read it in as well

		if(strcmp(ReadAlphabetName,AlphabetName.c_str()))
		{ // header indicates stored model uses a different alphabet
			delete[] ReadAlphabetName;
			return false;
		}
		delete[] ReadAlphabetName;

		int ReadNrNodes;
		bytesRead = fread(&ReadNrNodes,sizeof(ReadNrNodes), 1, InputFile);
		if(bytesRead < sizeof(ReadNrNodes) || ReadNrNodes != MaxNrNodes)
		{ // header indicates different number of nodes in the hashtable
			return false;
		}

		for(int i=0;i<MaxNrNodes;i++)
		{
			if(fread(&Tree[i].a,sizeof(CCTWNode::a),1,InputFile) < sizeof(CCTWNode::a)) return false;
			if(fread(&Tree[i].b,sizeof(CCTWNode::b),1,InputFile) < sizeof(CCTWNode::b)) return false;
			if(fread(&Tree[i].Symbol, sizeof(CCTWNode::Symbol),1,InputFile) < sizeof(CCTWNode::Symbol)) return false;
			if(fread(&Tree[i].NrTries, sizeof(CCTWNode::NrTries),1,InputFile) < sizeof(CCTWNode::NrTries)) return false;
			if(fread(&Tree[i].Pe, sizeof(CCTWNode::Pe),1,InputFile) < sizeof(CCTWNode::Pe)) return false;
			if(fread(&Tree[i].PwChild, sizeof(CCTWNode::PwChild),1,InputFile) < sizeof(CCTWNode::PwChild)) return false;
		}
		fclose(InputFile);
		return true;
	}
	else
		return false;
}

inline CLanguageModel::Context CCTWLanguageModel::CreateEmptyContext() {
    CCTWContext *pCont = new CCTWContext;
	return (Context) pCont;
}

inline CLanguageModel::Context CCTWLanguageModel::CloneContext(Context Copy) {
	CCTWContext *pCont = new CCTWContext;
    CCTWContext *pCopy = (CCTWContext *) Copy;

	pCont->Full = pCopy->Full;
	pCont->Context.assign(pCopy->Context.begin( ), pCopy->Context.end( ));

	return (Context) pCont;
}

inline void CCTWLanguageModel::ReleaseContext(Context release) {
	  delete (CCTWContext *) release;
}
