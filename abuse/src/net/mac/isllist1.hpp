#ifndef ISLLIST_HH
#define ISLLIST_HH

template <class T>
class isllist
{
protected:
	class list_node;
	typedef list_node *link;
	class list_node
	{
	public:
		link next;
		T data;
		
		list_node() {}
		list_node(const T& item) { data = item; }
	};
	
	link list;
	link end;
public:
	class iterator
	{
	friend class isllist;
	protected:
		link node;
		iterator(const link p) : node(p) {}
	public:
		iterator() {}
		iterator(const iterator &p) : node(p.node) {}

		int operator==(const iterator &p) const {	return (node == p.node); }
		int operator!=(const iterator &p) const { return (node != p.node); }
		
		iterator& operator++() { node = node->next; return *this;	}
		
		T& operator*() { return node->data; }
	};
	
	iterator end() 	 { return (link)(&list); }
	iterator begin() { return list; }
	
	int empty() { return begin() == end(); }
	
	iterator insert_next(iterator pos, const T& item)
	{
		link p = new list_node(item);
		p->next = pos.node->next;
		pos.node->next = p;
		
		return p; 
	}
	
	void erase_next(iterator pos)
	{
		link p = pos.node->next;
		pos.node->next = p->next;
		delete p;
	}
	
	int find(iterator& p, T& item)
	{
		while (p != end())
		{
			if (*p==item)
				return 1;
			++p;
		}
		return 0;
	}
	
	int find(T& item) { iterator p; return find(p, item); }
	void insert(const T& item) {	insert_next(end(), item); }
	void erase() { erase_next( end() ); }

	void erase_all()
	{
		while (!empty())
			erase();
	}
	
	isllist()
	{
		list = (link)&list;
	}
};

#endif