
class SimNeighbour : public Neighbour {
	public:
		Number x, y, z;
		Number lag;
		
		bool in_range;
		
		SimNeighbour(MachineId const & id, Size imports) : Neighbour(id, imports) {}
};

#undef Neighbour
#define Neighbour SimNeighbour
