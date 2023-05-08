%[y,Fs] = audioread("sinewave.wav");

y = zeros(1,512);
for i = 1:512;
  y(i) = round(32767*sin(2*pi*i/512));
end

out_file = fopen('sinewave.txt','w');
for i = 1:512;
  out_line = sprintf("%d, ",y(i));
  fprintf(out_file,"%s",out_line);
  if( 0 == mod(i,16) )
      fprintf(out_file,"\n");
  end
end
fclose(out_file);
