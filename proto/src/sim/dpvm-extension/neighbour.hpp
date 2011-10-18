
class SimNeighbour : public Neighbour {
	public:
		Counter data_age; // The number of rounds executed since the last update of the data of this neighbour.
		Number x, y, z;
		Number lag;
		
		bool in_range;
		
		SimNeighbour(MachineId const & id, Size imports) : Neighbour(id, imports) {}
};

#undef Neighbour
#define Neighbour SimNeighbour
