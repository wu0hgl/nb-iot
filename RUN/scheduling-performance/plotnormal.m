function plotnormal(ymatrix1, t, y, ymin, ymax)
%CREATEFIGURE1(YMATRIX1)
%  YMATRIX1:  bar matrix data

%  Auto-generated by MATLAB on 05-Mar-2013 21:48:14

% Create figure
figure1 = figure('Name', t);

% Create axes
axes1 = axes('Parent',figure1,'YMinorTick','on',...
    'YMinorGrid','on',...
    'XTickLabel',{'0.04','0.06','0.08','0.1'},...
    'XTick',[1 2 3 4],...
    'FontSize',20);
% Uncomment the following line to preserve the Y-limits of the axes
% ylim(axes1,[0.1 100]);
box(axes1,'on');
grid(axes1,'on');
hold(axes1,'all');

% Create multiple lines using matrix input to bar
bar1 = bar(ymatrix1,'BaseValue',ymin,'Parent',axes1);
set(bar1(1),'DisplayName','Proportional Fair');
set(bar1(2),'DisplayName','LOG rule');
set(bar1(3),'DisplayName','EXP rule');
set(bar1(4),'DisplayName','Frame Level Scheduler');

% Create xlabel
xlabel('Target Delay [s]','FontSize',20);

% Create ylabel
ylabel(y,'FontSize',20);

ylim ([ymin ymax])

% Create legend
legend1 = legend(axes1,'show');
set(legend1,'Location','NorthOutside');
saveas(figure1, t, 'png')

