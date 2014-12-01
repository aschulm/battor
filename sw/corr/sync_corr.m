function sync_corr(infile,battorfile)

HIGH_HZ = 55;
LOW_HZ = 45;
SAMP_RATE_HZ = 5000;
LED_MW=250;
TIMESYNC_PN = [1,-1,-1,1,-1,1,1,-1,1,1,-1,1,1,-1,-1,-1,1,1,-1,-1,1,1,1,1,1,-1,1,-1,1,1,-1,1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,1,1,-1,1,1,1,-1,1,1,1,-1,-1,-1,-1,-1,-1,1,1,-1,-1];
TIMESYNC_PN_SUB = LED_MW*[1,0,0,1,0,1,1,0,1,1,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,0,1,0,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1,0,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,1,1,0,0];
THRESH = 100000.0;

% bandpass filter
filt1 = designfilt('bandpassfir', ...
    'StopbandFrequency1',1,'PassbandFrequency1',LOW_HZ, ...
    'PassbandFrequency2',HIGH_HZ ,'StopbandFrequency2',SAMP_RATE_HZ/2 , ...
    'StopbandAttenuation1', 40,'PassbandRipple',1, ...
    'StopbandAttenuation2', 40,'DesignMethod','equiripple','SampleRate',SAMP_RATE_HZ);

% read in power measurements
file = fopen(infile);
% skip the comment lines
for i=1:5
    fgetl(file);
end
A = fscanf(file, '%f %f', [2 Inf]);
fclose(file);
A = A';

% read the timing of the timesyncs
S = [];
S_sub = [];
file = fopen(battorfile);
line = fgets(file);
while ischar(line)
    line = fgets(file);
    if (~isempty(strfind(line, 'sync')))
        s = sscanf(line, ['%*d sync ' repmat('%d ',1,64)], [64 Inf]);
        s = s';
        s = s - s(1);
        s = (s/1e6) * SAMP_RATE_HZ;
        s_samp = [];
        s_samp_sub = [];
        for i = 2:length(TIMESYNC_PN)
            % start missed bits
            if (s(i) < 0 && s(i-1) >= 0)
                missed_bits_start = i-1;
            % end missed bits
            elseif (s(i) >= 0 && s(i-1) < 0)
                samp = floor(s(i))-floor(s(missed_bits_start));
                s_samp = horzcat(s_samp, repmat(TIMESYNC_PN(missed_bits_start), 1, samp));
                s_samp_sub = horzcat(s_samp_sub, repmat(TIMESYNC_PN_SUB(missed_bits_start), 1, samp));
            elseif (s(i) >= 0 && s(i-1) >= 0)
                samp = floor(s(i))-floor(s(i-1));
                s_samp = horzcat(s_samp, repmat(TIMESYNC_PN(i-1), 1, samp));
                s_samp_sub = horzcat(s_samp_sub, repmat(TIMESYNC_PN_SUB(i-1), 1, samp));
            end
        end
        s_samp = horzcat(s_samp, repmat(-1,1,3200-length(s_samp))); % TODO make the 3200 clean
        s_samp_sub = horzcat(s_samp_sub, repmat(0,1,3200-length(s_samp_sub))); % TODO make the 3200 clean
        S = cat(1, S, s_samp(1:3200));
        S_sub = cat(1, S_sub, s_samp_sub(1:3200));
    end
end
fclose(file);

% power = v*i
A = bsxfun(@times, A(:,1), A(:,2));
A = A/1000.0;

A_filt = filtfilt(filt1,A);

% resample PN sequence to match sample rate
A_corr = kron(TIMESYNC_PN,ones(1,50)); % TODO make the 50 clean

%correlate power measuremnts with PN sequence
[C,C_lags]=xcorr(A_filt,A_corr);

% find the peaks in the correlation
[peaks, peakis] = findpeaks(abs(C), 'MinPeakDistance',50*64); % TODO make the 50 clean
I_peaks = find(peaks>THRESH);
A_hit = C_lags(peakis(I_peaks));

file_hits = fopen([infile '-hits'], 'w');
file_bits = fopen([infile '-bits'], 'w');

for x = kron(TIMESYNC_PN_SUB,ones(1,50))
  fprintf(file_bits, '0 %d\n', x);   
end

A_sub = A;
for i = 1:min(length(A_hit),size(S,1))
    hit = A_hit(i);   
    hit_time = S(i,:);
    [c,c_lags]=xcorr(A_filt(hit:hit+3199),hit_time);
    c_max_i = find(c==max(abs(c)));
    c_hit = c_lags(c_max_i);
    fprintf(file_hits, '%d %d\n', i, hit+c_hit);
    A_sub = A - cat(1, zeros(hit+c_hit-1,1), S_sub(i,:)', zeros(length(A_sub)-hit-c_hit-3199,1));
    
    % save bits
    for x = S_sub(i,:)
        fprintf(file_bits, '%d %d\n', i, x);   
    end
    
    % map each bit to its nearest zero crossing in the correct direction
    

    %plot(1:3200, S_sub(i,:)/20, 1:3200, [diff(A_filt(hit+c_hit:hit+c_hit+3199),2)' 0 0]);
    plot(1:3200, S(i,:)*150, 1:3200, A_filt(hit+c_hit:hit+c_hit+3199),1:3200,zeros(1,3200));
    
    pause;
    %plot(A(hit+c_hit:hit+c_hit+3199) - S_sub(i,:)');
    %pause;
end
fclose(file_hits);
fclose(file_bits);

file_sub = fopen([infile '-sub'], 'w');
for x = A_sub
    fprintf(file_sub, '%d\n', x); 
end
fclose(file_sub);

end
