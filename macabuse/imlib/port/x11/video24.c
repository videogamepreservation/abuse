
void x24_make_page(short width, short height, unsigned char *page_buffer)
{
  XImage_Info *xi;  
  if (special && !special->static_mem)
  {
    xi=new XImage_Info;
    special->extended_descriptor=(void *)xi;    
#ifndef NO_SHM
    if (doShm)
    {
      width=(width+3)&(0xffffffff-3);
      // create the image
      xi->XImg = XShmCreateImage(display,
				 X_visual,
				 24,
				 ZPixmap,
				 0,
				 &xi->X_shminfo,
				 width,
				 height );

      w=width=xi->XImg->bytes_per_line;  // adjust image size to X requirments
      
      // create the shared memory segment
      xi->X_shminfo.shmid = shmget (IPC_PRIVATE, width*height*3, IPC_CREAT | 0777);
      ERROR(xi->X_shminfo.shmid>=0,"shmget() failed, go figure");

      xi->X_shminfo.readOnly=False;
      

      // attach to the shared memory segment to us
      xi->XImg->data = xi->X_shminfo.shmaddr =
		(char *) shmat(xi->X_shminfo.shmid, 0, 0);
      ERROR(xi->XImg->data,"shmat() failed, go figure");

      if (page_buffer)
        memcpy(xi->XImg->data,page_buffer,width*height);           

      // get the X server to attach to it to the X server
      ERROR(XShmAttach(display, &xi->X_shminfo),"XShmAttach() failed, go figure");
      XSync(display,False); // make sure segment gets attached
      ERROR(shmctl(xi->X_shminfo.shmid,IPC_RMID,NULL)==0,"shmctl failed, why?"); 

    } else 
#endif
    {
      if (!page_buffer)
        page_buffer=(unsigned char *)jmalloc(width*height,"image::data");
      
      xi->XImg = XCreateImage(	display,
    				X_visual,
    				8, // my_visual->depth,
    				ZPixmap,
    				0,
    				(char *)page_buffer,
    				width, height,
    				8,
    				width );
      ERROR(xi->XImg,"XCreateImage failed");
    }
    data=(unsigned char *) (xi->XImg->data);        
  }
  else 
  {
    if (!page_buffer)
      data=(unsigned char *)jmalloc(width*height,"image::data");
    else data=page_buffer;
  }

  if (special)
    special->resize(width,height);
}
