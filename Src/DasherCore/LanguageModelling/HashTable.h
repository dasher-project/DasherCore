// HashTable.h
//



#pragma once

namespace Dasher {

  class CHashTable { //class to store the hashtable used to find indices of nodes	 
	public:
		CHashTable(){}		
		static int GetHashOffSet(int c){
			return Tperm[c];
		}	 
		private: 
			static const unsigned int Tperm[256];				
  };

} // end namespace 

