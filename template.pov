//povray +Wwidth +Hheight -Ithisfile -Ooutfile +FN16

#macro atom (coord, rad, col)
        sphere{coord, rad texture { pigment { rgb col }}}
#end

#macro bond (c1,c2, rad, col1,col2)
        cylinder{c1,(c1+c2)/2,rad texture { pigment { rgb col1 }}}
        cylinder{(c1+c2)/2,c2,rad texture { pigment { rgb col2 }}}
#end

#macro cell (c1,c2)
        cylinder{c1,c2,0.02 texture { pigment { rgb 0 }}}
#end

//fill this with data of viewport 
//HANDEDNESS!
camera {
  orthographic
  location <0.0000, 0.0000, -2.0000>
  look_at <0.0000, 0.0000, 2.0000>
  up <0.0000, 3.0000, 0.0000>
  right <2.4065, 0.0000, 0.0000>
}
light_source { 
  <-0.1000, 0.1000, -1.0000> 
  color rgb<1.000, 1.000, 1.000> 
  parallel 
  point_at <0.0, 0.0, 0.0> 
}
light_source { 
  <1.0000, 2.0000, -0.5000> 
  color rgb<1.000, 1.000, 1.000> 
  parallel 
  point_at <0.0, 0.0, 0.0> 
}
background {
  color rgb<1.000, 1.000, 1.000>
}
#default { texture {
 finish { ambient 0.000 diffuse 0.650 phong 0.1 phong_size 40.000 specular 0.500 }
} }

//append atom/bond data
atom(<0.0007,-0.3564,-0.0623>,0.2479,rgbt<0.250,0.750,0.750,0.000>)
atom(<-0.0014,0.5020,0.0554>,0.2479,rgbt<0.250,0.750,0.750,0.000>)
atom(<0.0015,-0.6780,0.4993>,0.1458,rgbt<1.000,1.000,1.000,0.000>)
atom(<-0.5186,-0.5567,-0.3822>,0.1458,rgbt<1.000,1.000,1.000,0.000>)
atom(<0.5216,-0.5541,-0.3816>,0.1458,rgbt<1.000,1.000,1.000,0.000>)
atom(<-0.5457,0.8213,0.1332>,0.1458,rgbt<1.000,1.000,1.000,0.000>)
atom(<0.5419,0.8220,0.1382>,0.1458,rgbt<1.000,1.000,1.000,0.000>)
bond(<0.0007,-0.3564,-0.0623>,<-0.0014,0.5020,0.0554>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<0.250,0.750,0.750,0.000>)
bond(<0.0007,-0.3564,-0.0623>,<0.0015,-0.6780,0.4993>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<1.000,1.000,1.000,0.000>)
bond(<0.0007,-0.3564,-0.0623>,<-0.5186,-0.5567,-0.3822>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<1.000,1.000,1.000,0.000>)
bond(<0.0007,-0.3564,-0.0623>,<0.5216,-0.5541,-0.3816>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<1.000,1.000,1.000,0.000>)
bond(<-0.0014,0.5020,0.0554>,<-0.5457,0.8213,0.1332>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<1.000,1.000,1.000,0.000>)
bond(<-0.0014,0.5020,0.0554>,<0.5419,0.8220,0.1382>,0.0437,rgbt<0.250,0.750,0.750,0.000>,rgbt<1.000,1.000,1.000,0.000>)
