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
public:
	class iterator
	{
	friend class isllist;
	public:
		// pseudo-protected - don't use unless you really have to
		link node;
		iterator(const link p) : node(p) {}
	public:
		iterator() {}
		iterator(const iterator &p) : node(p.node) {}

		int operator==(const iterator &p) const {	return (node == p.node); }
		int operator!=(const iterator &p) const { return (node != p.node); }
		
		iterator& operator++() { node = node->next; return *this;	}
		iterator next() { return node->next; }
		
		T& operator*() { return node->data; }
	};
	
	iterator end() 	 { return (link)(&list); }
	iterator begin_prev() { return end(); }
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
	
	int find_prev(iterator& p, const T& item)
	{
		while (p.node->next != end().node)
		{
			if (*(p.next())==item)
				return 1;
			++p;
		}
		return 0;
	}
	
	void move_next(iterator&p, iterator&q)
	{
		link tmp;
		
		tmp = p.node->next;
		if (tmp == q.node)
			return;
		p.node->next = tmp->next;
		tmp->next = q.node->next;
		q.node->next = tmp;
	}
	
	int find(T& item) { iterator p = begin_prev(); return find_prev(p, item); }
	void insert(const T& item) {	insert_next( begin_prev(), item); }
	void erase() { erase_next( begin_prev() ); }

	void erase_all()
	{
		while (!empty())
			erase();
	}
	
	isllist()
	{
		list = (link)&list;
	}
	
	~isllist()
	{
		erase_all();
	}
};

#endif