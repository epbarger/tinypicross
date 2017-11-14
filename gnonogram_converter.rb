puzzles = []
Dir.glob('gnonogram_saves/*.gno').sort_by { |fn| fn.match(/\d+/)[0].to_i }.each do |gno_file|
  output = `cat #{gno_file} | grep "Solution" -A 7`
  puzzle = output.split("\n")[1..-1]
  puzzle = puzzle.map { |row_str| row_str.split(" ").map{ |num| num.to_i - 1 } }
  puzzles << puzzle
end

File.open("puzzles.h.generated", "w") do |file|
  file.puts("#define PUZZLE_COUNT #{puzzles.size}\n\n")
  file.puts("const byte puzzles[PUZZLE_COUNT][15] PROGMEM = {") 
  puzzles.each_with_index do |p, p_index|
    file.print("  {")
    15.times do |i|
      file.print("0b#{p[0][i]}#{p[1][i]}#{p[2][i]}#{p[3][i]}#{p[4][i]}#{p[5][i]}#{p[6][i]}#{i + 1 == 15 ? '' : ', '}")
    end
    file.puts("},")
  end
  file.puts("};")
end
