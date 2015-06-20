#ifndef __SPRITE_HPP
#define __SPRITE_HPP
#include "macs.hpp"
#include "image.hpp"
#include "linked.hpp"

class sprite : public linked_node
{
public:
  image *visual,*screen,*save;
  int x,y;
  sprite(image *Screen, image *Visual, int X, int Y);
  void get_background();
  void restore_background();
  void draw();
  void change_visual(image *Visual, int delete_old=0);
  ~sprite();
} ;

class sprite_controller
{
public :
  linked_list sprites;
  void add_sprite(sprite *sp);
  void remove_sprites();
  void get_backgrounds();
  void put_sprites();
  void bring_front(sprite *sp);
  void delete_sprite(sprite *sp);
} ;
#endif

