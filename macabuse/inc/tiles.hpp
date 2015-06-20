class tile
{
  uchar *im_data;
  uchar *run_data;
  boundary *points;
  ushort next;
  public :
  tile(bFILE *fp, int type);
} ;


tile::tile(bFILE *fp, int type, int w, int h)
{ 
  int cw=fp->read_short(),ch=fp->read_short();
  if (cw!=w || ch!=h)
  {
    lbreak("load_tiles : expecting tile size to be %dx%d, got %dx%d\n",w,h,cw,ch);
    exit(0);
  }

  im_data=(uchar *)jmalloc(w*h,"tile image");
  fp->read(im_data,w*h);
  
  if (type==SPEC_FORETILE)
  {
    next=fp->read_short();  // next
    fp->read_byte();        // skip damage, not implemented
    points=new boundary(fp,"tile boundary");
    uchar *c=im_data;
    int need_runs=0;

    if (need_runs)
      run_data=make_runs(im_data,w,h);
    } else run_data=NULL;
  } else { points=NULL; run_data=NULL; }    
}

class tile_set
{
  public :
  int w,h,t;
  int *id;
  tile_set *next;
  tile_set(int width, int height);  
  void add(int tile_id, int tile_number);
} ;


tile_set::tile_set(int width, int height, tile_set *Next)
{
  w=width;
  h=height;
  t=0;
  next=Next;
  id=NULL;
}


void tile_set::add(int tile_id, int tile_number)
{
  if (tile_number>=t)
  {
    id=(int *)jrealloc(id,sizeof(int)*tile_number,"tile set list");
    t=tile_number;
  }
  id[tile_number]=tile_id;    
}



